using Microsoft.Data.Sqlite;
using System.Runtime.InteropServices;
using System.Runtime.CompilerServices;
using System.Collections.Concurrent;

namespace cads_gui.BeltN;
public static class Search
{

  public static float[] ConvertBytesToFloats(byte[] a)
  {

    var z_ptr = new ReadOnlySpan<byte>(a);
    var j = MemoryMarshal.Cast<byte, float>(z_ptr);
    return j.ToArray();

  }

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

    while (await reader.ReadAsync(stop))
    {
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
      var z = ConvertBytesToFloats((byte[])reader[0]);
      return z;
    }

    var rows = DBReadQuery(db, CmdBuilder, Read, stop);

    await foreach (var r in rows)
    {
      yield return r;
    }
  }

  public static async Task<(double, double, double, double, double)> RetrieveBeltAttributesAsync(string db, CancellationToken stop = default)
  {

    SqliteCommand CmdBuilder(SqliteCommand cmd)
    {
      var query = $"select length(z),(select count(rowid) from PROFILE),y,x_off from PROFILE where rowid = 1";
      cmd.CommandText = query;
      return cmd;
    }

    (double, double, double, double, double) Read(SqliteDataReader reader)
    {

      var width = reader.GetDouble(0) / sizeof(float);
      var length = reader.GetDouble(1);
      var xRes = 1.0d;
      var yRes = reader.GetDouble(2);
      var xOff = reader.GetDouble(3);
      return (width, length, xRes, yRes, xOff);
    }

    return await DBReadQuerySingle(db, CmdBuilder, Read, stop);
  }

  public static (int, float) CountIf(ReadOnlySpan<float> sq, Func<int, bool> fx, Func<float, bool> fz)
  {

    var zMin = float.MaxValue;
    var cnt = 0;
    var x = 0;

    foreach (var z in sq)
    {
      if (!float.IsNaN(z) && fz(z) && fx(x++))
      {
        zMin = Math.Min(zMin, z);
        cnt++;
      }
    }

    return (cnt, zMin);
  }

  public static async IAsyncEnumerable<(int, float, int)> CountIf(IAsyncEnumerable<float[]> rows, int stride, Func<int, bool> fx, Func<float, bool> fz, [EnumeratorCancellation] CancellationToken stop = default)
  {

    await foreach (var r in rows.WithCancellation(stop))
    {
      ReadOnlyMemory<float> z = r;
      int i = 0;

      for (; i < z.Length; i += stride)
      {
        var len = Math.Min(stride, z.Length - i);
        var p = z.Slice(i, len);
        var (cnt, zMin) = CountIf(p.Span, fx, fz);
        yield return (cnt, zMin, len);
      }

    }
  }

  public static async IAsyncEnumerable<(int, float, int)> CountIfMatrixAsync(IAsyncEnumerable<(int, float, int)> belt, int columns, long rows, [EnumeratorCancellation] CancellationToken stop = default)
  {

    var columnPartition = new (int, float, int)[columns];
    Array.Fill(columnPartition, (0, float.MaxValue, 0));
    var hasColumns = false;

    long i = 0;

    await foreach (var r in belt.WithCancellation(stop))
    {
      hasColumns = true;
      var im = i++ % columns;
      columnPartition[im] = (columnPartition[im].Item1 + r.Item1, Math.Min(columnPartition[im].Item2, r.Item2), columnPartition[im].Item3 + r.Item3);

      if (i > 0 && i % (columns * rows) == 0)
      {
        foreach (var e in columnPartition)
        {
          yield return e;
        }

        Array.Fill(columnPartition, (0, float.MaxValue, 0));
        hasColumns = false;
      }
    }

    if (hasColumns)
    {
      foreach (var e in columnPartition)
      {
        yield return e;
      }
    }

  }


  public static async IAsyncEnumerable<(double, double, int, float, int)> AddCoordinatesAsync(IAsyncEnumerable<(int, float, int)> areas, int columns, double yOrigin, [EnumeratorCancellation] CancellationToken stop = default)
  {

    long t = 0;

    await foreach (var area in areas.WithCancellation(stop))
    {
      var x = t % columns;
      var y = t / columns;
      t++;
      yield return (x, y + yOrigin, area.Item1, area.Item2, area.Item3);
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

  public static async IAsyncEnumerable<(double, double, double, float)> SearchPartitionAsync(string db, int columns, int rows, long y, int width, long length, long limit, Func<int, bool> fx, Func<float, bool> fz, Func<(double, double, double, float), bool> fp, [EnumeratorCancellation] CancellationToken cancel = default)
  {
    var xStride = (int)Math.Ceiling((double)width / columns);
    var yStride = (long)Math.Ceiling((double)length / rows);
    var ySkipped = (long)Math.Floor((double)y / yStride);
    var rawSamples = Search.RetrieveFrameSamplesAsync(y, length, db, cancel);
    var g = Search.CountIf(rawSamples, xStride, fx, fz, cancel);
    var l = Search.CountIfMatrixAsync(g, columns, yStride,  cancel);
    var ll = Search.AddCoordinatesAsync(l, columns, (double)y,cancel);

    await foreach (var m in Filter(Map(Take(ll, limit, cancel), (a) => (a.Item1, a.Item2, (double)a.Item3 / a.Item5, a.Item4), cancel), fp, cancel).WithCancellation(cancel))
    {
      yield return m;
    }
  }

  public static async Task<IEnumerable<(double, double, double, float)>> SearchPartitionParallelAsync(string db, int columns, int rows, int width, long length, long limit, Func<int, bool> fx, Func<float, bool> fz, Func<(double, double, double, float), bool> fp, CancellationToken cancel = default)
  {
    var procn = (int)Math.Min(Environment.ProcessorCount, rows);

    long yPartitionLength = length / procn;
    var rowsPartition = (int)Math.Floor((double)rows / procn);
    var yOffsets = (from number in Enumerable.Range(0, procn - 1) select (number * yPartitionLength, yPartitionLength,rowsPartition)).Append(((procn - 1) * yPartitionLength, length - (procn - 1) * yPartitionLength,rows - (procn - 1)*rowsPartition));
    var results = new ConcurrentBag<(double, double, double, float)>();
    var dgn = yOffsets.ToList();
    await Parallel.ForEachAsync(yOffsets, async (yOffset, stop) =>
    {
      var cancellation = CancellationTokenSource.CreateLinkedTokenSource(stop, cancel);
      await foreach (var m in SearchPartitionAsync(db, columns, yOffset.Item3, yOffset.Item1, width, yOffset.Item2, limit, fx, fz, fp, cancellation.Token).WithCancellation(cancellation.Token))
      {
        results.Add(m);
      }
    });

    return results.AsEnumerable();
  }

} // class Search
