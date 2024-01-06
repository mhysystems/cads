using System;
using System.Linq;
using System.Collections.Generic;
using Microsoft.Data.Sqlite;
using System.Runtime.InteropServices;
using System.Diagnostics;
using System.Data;
using System.Threading.Tasks;
using System.Runtime.CompilerServices;

using SQLitePCL;
using static SQLitePCL.raw;
using CadsFlatbuffers;



namespace cads_gui.Data
{

  public record Profile<T>(double Y, T[] Z);
  public record Profile(double y, float[] z);

  public static class NoAsp
  {

    public static byte[] ConvertFloatsToBytes(float[] a)
    {
      var z_ptr = new ReadOnlySpan<float>(a);
      var j = MemoryMarshal.Cast<float, byte>(z_ptr);
      return j.ToArray();
    }

    public static float[] ConvertBytesToFloats(byte[] a)
    {
      var z_ptr = new ReadOnlySpan<byte>(a);
      var j = MemoryMarshal.Cast<byte, float>(z_ptr);
      return j.ToArray();
    }

    static R DBReadQuerySingle<R>(string name, Func<SqliteCommand, SqliteCommand> cmdBuild, Func<SqliteDataReader, R> rows)
    {
      using var connection = new SqliteConnection("" +
          new SqliteConnectionStringBuilder
          {
            Mode = SqliteOpenMode.ReadOnly,
            DataSource = name
          });

      connection.Open();

      var command = cmdBuild(connection.CreateCommand());

      using var reader = command.ExecuteReader();
      reader.Read();
      return rows(reader);

    }

    public static async Task<List<Profile>> RetrieveFrameModular(string db, double y_min, long len, long left)
    {

      if (!File.Exists(db))
      {
        return Enumerable.Empty<Profile>().ToList();
      }

      var frame = new List<Profile>();
      var abslen = Math.Abs(len);

      using var connection = new SqliteConnection("" +
        new SqliteConnectionStringBuilder
        {
          Mode = SqliteOpenMode.ReadOnly,
          DataSource = db
        });

      await connection.OpenAsync();

      var commandRes = connection.CreateCommand();
      commandRes.CommandText = $"select Belt.Length, max(Profiles.rowid)+1, Meta.ZEncoding, Gocator.zResolution from Belt join Meta join Profiles join Gocator limit 1;";

      using var readerRes = commandRes.ExecuteReader(CommandBehavior.SingleRow);

      readerRes.Read();
      var max = readerRes.GetInt64(1);
      var y_res = readerRes.GetDouble(0) / max;
      var ZEncoding = readerRes.GetInt64(2);
      var zResolution = readerRes.GetDouble(3);
     

      readerRes.Close();

      var row = (long)Math.Floor(y_min / y_res);
      var rowBegin = Mod(row - left, max);
      var mask = rowBegin > row - left || Mod(row - left + len, max) < row - left + len ? 1.0 : 0.0;


      var query = $"select (rowid - @mask*@rowmax)*@yres,Z from Profiles where rowid >= @row union all select rowid*@yres,Z from Profiles limit @len";
      var command = connection.CreateCommand();
      command.CommandText = query;
      command.Parameters.AddWithValue("@row", rowBegin);
      command.Parameters.AddWithValue("@len", abslen);
      command.Parameters.AddWithValue("@mask", mask);
      command.Parameters.AddWithValue("@rowmax", max);
      command.Parameters.AddWithValue("@yres", y_res);


      using var reader = command.ExecuteReader();


      while (reader.Read())
      {
        var y = reader.GetDouble(0);
        var z =  ZEncoding switch 
        {
          1 => ZbitUnpacking((byte[])reader[1],(float)zResolution),
          _ => ConvertBytesToFloats((byte[])reader[1])
        };

        if (len >= 0)
        {
          frame.Add(new Profile(y, z));
        }
        else
        {
          frame.Insert(0, new Profile(y, z));
        }
      }

      return frame;
    }


    public static double Mod(double x, double n) { return ((x % n) + n) % n; }
    static float[] Convert(byte[] a)
    {

      ReadOnlySpan<byte> z_ptr = new(a);
      var j = MemoryMarshal.Cast<byte, float>(z_ptr);
      return j.ToArray();

    }

    public static int[] UnpackZBits(byte[] z) 
    {
      ReadOnlySpan<byte> zPtr = new(z);
      var header = MemoryMarshal.Cast<byte, int>(zPtr);
      var length = header[0];
      var bits = header[1];
      int mask = (1 << bits) - 1 ;

      int[] Z = new int[length];
      
      Z[0] = header[2];

      var packedZs = MemoryMarshal.Cast<byte, int>(zPtr.Slice(sizeof(int)*3));

      int indexZ = 1;
      int packedIndex = 0;
      int i = bits;

      int maxBits = sizeof(int)*8;

      while (indexZ < length)
      {

        if(i < maxBits) {
          Z[indexZ++] = (packedZs[packedIndex] << (maxBits - i)) >> (maxBits - bits);
        }else {
          Z[indexZ] = packedZs[packedIndex++] >> (i - bits);
          i -= maxBits;
          Z[indexZ++] |= (packedZs[packedIndex] << (maxBits - i)) >> (maxBits - bits);
        }

        i += bits;
        
      }

      return Z;
    }

    public static float[] Unquantise(int[] z, float res)
    {
      float[] Z = new float[z.Length];
	    
      for(var i = 0; i< Z.Length; i++) {
		    Z[i] = z[i] != Int32.MinValue ? z[i] * res : Single.NaN;
	    }

	    return Z;
    }

    public static float [] DeltaDecoding(float[] z)
    {
	    float zn = 0;
      for(var i = 0; i< z.Length; i++) {
		    z[i] = z[i] + zn;
		    zn = z[i];
	    }

	    return z;
    }

    public static float[] ZbitUnpacking(byte[] z, float res)
    {
      try {
      return DeltaDecoding(Unquantise(UnpackZBits(z),res));
      }catch(Exception e) {
        return new float[1];
      }
    }

    public static (bool, float) RetrievePoint(string db, double y, long x)
    {
      if (!File.Exists(db))
      {
        return (true, 0.0f);
      }

      using var connection = new SqliteConnection("" +
        new SqliteConnectionStringBuilder
        {
          Mode = SqliteOpenMode.ReadOnly,
          DataSource = db,
        });

      if (connection is null)
      {
        return (true, 0.0f);
      }

      connection.Open();

      var command = connection.CreateCommand();

      if (command is null)
      {
        return (true, 0.0f);
      }

      command.CommandText = $"select Y from Profiles where Y > 0 limit 1";

      using var reader = command.ExecuteReader(CommandBehavior.SingleRow);
      var y_res = 0.0;

      if (reader.Read())
      {
        y_res = reader.GetDouble(0);
      }
      else
      {
        return (true, 0.0f);
      }
      reader.Close();

      command.CommandText = $"select Z from Profiles where rowid = @rowid";
      command.Parameters.AddWithValue("@rowid", (int)(y / y_res));

      using var reader2 = command.ExecuteReader(CommandBehavior.SingleRow);

      if (reader2 is null)
      {
        return (true, 0.0f);
      }

      if (reader2.Read())
      {

        byte[] z = (byte[])reader2[0];
        var j = Convert(z);
        var r = j[x];
        return (false, r);
      }
      else
      {
        return (true, 0.0f);
      }

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
        var query = $"select Z from Profiles where rowid >= @rowid limit @len";
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

    public static ScanLimits? GetScanLimits(string filePath)
    {
      SqliteCommand CmdBuilder(SqliteCommand cmd)
      {
        var query = $"select belt.Width, belt.WidthN as WidthN, belt.Length as Length, max(Profiles.rowid)+1 as LengthN, limits.ZMin, limits.ZMax from Profiles join belt join limits limit 1";
        cmd.CommandText = query;
        return cmd;
      }

      ScanLimits Read(SqliteDataReader reader)
      {
        return new ScanLimits(reader.GetDouble(0),
          reader.GetInt64(1),
          reader.GetDouble(2),
          reader.GetInt64(3),
          reader.GetDouble(4),
          reader.GetDouble(5));
      }

      return DBReadQuerySingle(filePath, CmdBuilder, Read);
    }

    public static (string site, string belt, DateTime chronos) DecontructSQliteDbName(string filename)
    {
      var p = filename.Split("-");
      var site = p[0];
      var belt = p[1];
      var c = string.Join("-", p[2..^1]);
      var chrono = DateTime.ParseExact(c, "yyyy-MM-dd-HHmms", System.Globalization.CultureInfo.InvariantCulture);

      return (site, belt, chrono);
    }

    public static string MakeScanFilename(string site, string conveyor, DateTime chrono)
    {
      return site + '-' + conveyor + '-' + chrono.ToString("yyyy-MM-dd-HHmms");
    }

    public static Conveyor FromFlatbuffer(CadsFlatbuffers.Conveyor conveyor)
    {
      return new Conveyor()
      {
        Site = conveyor.Site,
        Name = conveyor.Name,
        Timezone = TimeZoneInfo.FindSystemTimeZoneById(conveyor.Timezone),
        PulleyCircumference = conveyor.PulleyCircumference,
        TypicalSpeed = conveyor.TypicalSpeed
      };
    }

    public static Belt FromFlatbuffer(CadsFlatbuffers.Belt belt)
    {
      return new Belt()
      {
        Serial = belt.Serial,
        PulleyCover = belt.PulleyCover,
        CordDiameter = belt.CordDiameter,
        TopCover = belt.TopCover,
        Length = belt.Length,
        Width = belt.Width
      };
    }

  } // End class NoAsp

} // End namespace