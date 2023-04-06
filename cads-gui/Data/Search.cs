using Microsoft.Data.Sqlite;
using System.Runtime.InteropServices;
using System.Runtime.CompilerServices;
using System.Collections.Concurrent;
using cads_gui.Data;

namespace cads_gui.BeltN;
public static class Search
{

  public record ZMinCoord(long X, long Y, float Z);
  public record Result(int Count, int Width, int Length, ZMinCoord ZMin);
  public record ResultCoord(long Col, long Row, Result Result);
  public record SearchResult(long Col, long Row, long Width, long Length, double Percent, ZMinCoord ZMin);

  public static async IAsyncEnumerable<R> DBReadQuery<R>(string name, Func<SqliteCommand, SqliteCommand> cmdBuild, Func<SqliteDataReader, R> rows, [EnumeratorCancellation] CancellationToken stop = default)
  {
    using var connection = new SqliteConnection("" +
        new SqliteConnectionStringBuilder
        {
          Mode = SqliteOpenMode.ReadOnly,
          DataSource = name
        });

    await connection.OpenAsync(stop);

    var command = cmdBuild(connection.CreateCommand());

    using var reader = await command.ExecuteReaderAsync(stop);
    int a = 0;
    while (await reader.ReadAsync(stop))
    {
      a++;
      yield return rows(reader);
    }
  }

  public static async Task<R> DBReadQuerySingle<R>(string name, Func<SqliteCommand, SqliteCommand> cmdBuild, Func<SqliteDataReader, R> rows, CancellationToken stop = default)
  {
    using var connection = new SqliteConnection("" +
        new SqliteConnectionStringBuilder
        {
          Mode = SqliteOpenMode.ReadOnly,
          DataSource = name
        });

    await connection.OpenAsync(stop);

    var command = cmdBuild(connection.CreateCommand());

    using var reader = await command.ExecuteReaderAsync(stop);
    await reader.ReadAsync(stop);
    return rows(reader);

  }

  public static async IAsyncEnumerable<float[]> RetrieveFrameSamplesAsync(long rowid, long len, string db, [EnumeratorCancellation] CancellationToken stop = default)
  {

    SqliteCommand CmdBuilder(SqliteCommand cmd)
    {
      var query = $"select z from PROFILE where rowid >= @rowid limit @len";
      cmd.CommandText = query;
      cmd.Parameters.AddWithValue("@rowid", rowid);
      cmd.Parameters.AddWithValue("@len", len);
      return cmd;
    }

    float[] Read(SqliteDataReader reader)
    {
      var z = NoAsp.ConvertBytesToFloats((byte[])reader[0]);
      return z;
    }

    var rows = DBReadQuery(db, CmdBuilder, Read, stop);

    await foreach (var r in rows)
    {
      yield return r;
    }
  }

  public static (int, ZMinCoord) CountIf(ReadOnlySpan<float> sq, Func<int, bool> fx, Func<float, bool> fz, int x)
  {

    var zMin = new ZMinCoord(0,0,float.MaxValue);
    var cnt = 0;
    var i = 0;

    foreach (var z in sq)
    {
      if (!float.IsNaN(z) && fz(z) && fx(x+i))
      {
        if(z < zMin.Z) {
          zMin = zMin with {X = i, Z = z};
        }
        cnt++;
      }

      i++;
    }

    return (cnt, zMin);
  }

  public static async IAsyncEnumerable<Result> CountIf(IAsyncEnumerable<float[]> rows, int stride, Func<int, bool> fx, Func<float, bool> fz, [EnumeratorCancellation] CancellationToken stop = default)
  {

    await foreach (var r in rows.WithCancellation(stop))
    {
      ReadOnlyMemory<float> z = r;
      int i = 0;

      for (; i < z.Length; i += stride)
      {
        var len = Math.Min(stride, z.Length - i);
        var p = z.Slice(i, len);
        var (cnt, zMin) = CountIf(p.Span, fx, fz, i);
        yield return new (cnt, len, 1, zMin with {X = 0 + zMin.X});
      }

    }
  }

  public static async IAsyncEnumerable<Result> CountIfMatrixAsync(IAsyncEnumerable<Result> belt, int columns, long stride, [EnumeratorCancellation] CancellationToken stop = default)
  {

    var columnPartition = new Result[columns];
    Array.Fill(columnPartition, new (0, 0, 0, new (0,0,float.MaxValue)));
    var hasColumns = false;

    long i = 0;

    await foreach (var r in belt.WithCancellation(stop))
    {
      hasColumns = true;
      var x = i % columns;
      var y = i / columns % stride;
      var zMin = columnPartition[x].ZMin;
      
      if(r.ZMin.Z < columnPartition[x].ZMin.Z) {
        zMin = r.ZMin with {Y = y};
      }
      
      var p = columnPartition[x];
      p = p with {Count = p.Count + r.Count, Width = r.Width, Length =  p.Length + r.Length, ZMin = zMin};
      columnPartition[x] = p;

      if (i > 0 && (i+1) % (columns * stride) == 0)
      {
        foreach (var e in columnPartition)
        {
          yield return e;
        }

        Array.Fill(columnPartition, new (0, 0, 0, new (0,0,float.MaxValue)));
        hasColumns = false;
      }
      
      i++;
    }

    if (hasColumns)
    {
      foreach (var e in columnPartition)
      {
        yield return e;
      }
    }

  }


  public static async IAsyncEnumerable<ResultCoord> AddCoordinatesAsync(IAsyncEnumerable<Result> areas, int columns, long yOrigin, [EnumeratorCancellation] CancellationToken stop = default)
  {

    long t = 0;

    await foreach (var area in areas.WithCancellation(stop))
    {
      var x = t % columns;
      var y = t / columns;
      t++;
      yield return new ResultCoord(x, y, area);// with {ZMin = area.ZMin with {Y = area.ZMin.Y}});
    }
  }

  public static async IAsyncEnumerable<ResultCoord> AddCoordinatesAsync2(IAsyncEnumerable<Result> areas, int columns, long xStride, long yStride, long yOrigin, [EnumeratorCancellation] CancellationToken stop = default)
  {

    long t = 0;

    await foreach (var area in areas.WithCancellation(stop))
    {
      var x = t % columns;
      var y = t / columns;
      t++;
      var zMin = area.ZMin with {X = x*xStride + area.ZMin.X, Y = y*yStride + area.ZMin.Y + yOrigin};
      yield return new ResultCoord(x*xStride, y*yStride + yOrigin, area with {ZMin = zMin});// with {ZMin = area.ZMin with {Y = area.ZMin.Y}});
    }
  }

  public static async IAsyncEnumerable<T> Filter<T>(IAsyncEnumerable<T> areas, Func<T, bool> fn, [EnumeratorCancellation] CancellationToken stop = default)
  {

    await foreach (var area in areas.WithCancellation(stop))
    {
      if (fn(area))
      {
        yield return area;
      }
    }
  }

  public static async IAsyncEnumerable<R> Map<T, R>(IAsyncEnumerable<T> areas, Func<T, R> fn, [EnumeratorCancellation] CancellationToken stop = default)
  {

    await foreach (var area in areas.WithCancellation(stop))
    {
      yield return fn(area);
    }
  }

  public static async IAsyncEnumerable<T> Take<T>(IAsyncEnumerable<T> areas, long take, [EnumeratorCancellation] CancellationToken cancel = default)
  {
    long limit = take;
    var takeCancellation = new CancellationTokenSource();
    var stop = CancellationTokenSource.CreateLinkedTokenSource(takeCancellation.Token, cancel);

    await foreach (var area in areas.WithCancellation(stop.Token))
    {
      if (limit-- > 0)
      {
        yield return area;
      }
      else
      {
        stop.Cancel();
        yield break;
      }
    }
  }


  public static async IAsyncEnumerable<SearchResult> SearchPartitionAsync(string db, int xStride, int yStride, long y, int width, long length, long limit, Func<int, bool> fx, Func<float, bool> fz, Func<SearchResult, bool> fp, [EnumeratorCancellation] CancellationToken cancel = default)
  {
    var columns = (int)Math.Ceiling((double)width / xStride);
    var rows = (long)Math.Ceiling((double)length / yStride);
    var rawSamples = RetrieveFrameSamplesAsync(y, length, db, cancel);
    var g = CountIf(rawSamples, xStride, fx, fz, cancel);
    var l = CountIfMatrixAsync(g, columns, yStride,  cancel);
    var ll = AddCoordinatesAsync2(l, columns, xStride, yStride, y,cancel);

    static SearchResult map(ResultCoord a)
    {
      return new SearchResult(a.Col,a.Row,a.Result.Width,a.Result.Length,(double)a.Result.Count / (a.Result.Width * a.Result.Length),a.Result.ZMin);
    }

    await foreach (var m in Take(Filter(Map(ll, map, cancel), fp, cancel),limit,cancel).WithCancellation(cancel)         )
    {
      yield return m;
    }
  }


  public static async Task<IEnumerable<SearchResult>> SearchParallelAsync(string db, int columns, int rows, int width, long length, long limit, Func<int, bool> fx, Func<float, bool> fz, Func<SearchResult, bool> fp, CancellationToken cancel = default)
  {
    var procn = (int)Math.Min(Environment.ProcessorCount, rows);

    var xStride = (int)Math.Ceiling((double)width / columns);
    var yStride = (int)Math.Ceiling((double)length / rows);
    long yPartitionLength = length / procn;
    var rowsPartition = (int)Math.Floor((double)rows / procn);
    var yOffsets = (from number in Enumerable.Range(0, procn - 1) select (number * yPartitionLength, yPartitionLength,rowsPartition)).Append(((procn - 1) * yPartitionLength, length - (procn - 1) * yPartitionLength,rows - (procn - 1)*rowsPartition));
    var results = new ConcurrentBag<SearchResult>();
    var dgn = yOffsets.ToList();
    await Parallel.ForEachAsync(yOffsets, async (yOffset, stop) =>
    {
      var cancellation = CancellationTokenSource.CreateLinkedTokenSource(stop, cancel);
      await foreach (var m in SearchPartitionAsync(db, xStride, (int)Math.Min(yStride,yOffset.Item2), yOffset.Item1, width, yOffset.Item2, limit, fx, fz, fp, cancellation.Token).WithCancellation(cancellation.Token))
      {
        results.Add(m);
      }
    });

    return results.AsEnumerable();
  }

} // class Search
