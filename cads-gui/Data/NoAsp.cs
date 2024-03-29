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
using Microsoft.EntityFrameworkCore.Storage.ValueConversion;



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
        var z = SelectZDecoder((int)ZEncoding)((byte[])reader[1],(float)zResolution);

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
      var packedZs = MemoryMarshal.Cast<byte, int>(zPtr);
      var length = packedZs[0];
      var bits = packedZs[1];
      var min = packedZs[2];
      int mask = (1 << bits) - 1 ;

      int[] Z = new int[length];
      
      Z[0] = packedZs[3];

      int indexZ = 1;
      int packedIndex = 4;

      int l = 0;
      int r = 0;
      int i = 0;

      int maxBits = sizeof(int)*8;

      while (indexZ < length)
      {

        Z[indexZ] |= (int)(((uint)packedZs[packedIndex]  << l ) >>> r) & mask ;

        i += bits;
        Z[indexZ] += min*System.Convert.ToInt32(i <= maxBits);
        indexZ += System.Convert.ToInt32(i <= maxBits);
        packedIndex += System.Convert.ToInt32(i >= maxBits);
        l = (maxBits - i + bits)*System.Convert.ToInt32(i > maxBits);
        r = i*System.Convert.ToInt32(i < maxBits);
        i = i - maxBits*System.Convert.ToInt32(i >= maxBits) - bits*System.Convert.ToInt32(l > 0);

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

    public static int [] DeltaDecoding(int[] z)
    {
	    int zn = 0;
      for(var i = 0; i< z.Length; i++) {
		    z[i] = z[i] + zn;
		    zn = z[i];
	    }

	    return z;
    }

    public static float[] ZbitUnpacking(byte[] z, float res)
    {
      return Unquantise(DeltaDecoding(UnpackZBits(z)),res);
    }

    public static Func<byte[],float,float[]> SelectZDecoder(int zEncoding)
    {
      return zEncoding switch 
        {
          1 => ZbitUnpacking,
          _ => (b,_) => ConvertBytesToFloats(b)
        };
    }

    public static (bool, float) RetrievePoint(string db, double y, long x)
    {
      if (!File.Exists(db))
      {
        return (true, 0.0f);
      }

      SqliteCommand CmdEncoding(SqliteCommand cmd)
      {
        var query = $"select ZEncoding, ZResolution, Belt.Length / (max(Profiles.rowid)+1) from Meta join Gocator join Belt join Profiles limit 1";
        cmd.CommandText = query;
        return cmd;
      }

      (Int32,float,double) ReadEncoding(SqliteDataReader reader)
      {
        return (reader.GetInt32(0),reader.GetFloat(1),reader.GetDouble(2));
      }
      
      var (ZEncoding,ZResolution,yRes) = DBReadQuerySingle(db, CmdEncoding, ReadEncoding);
      
      
      SqliteCommand CmdBuilder(SqliteCommand cmd)
      {
        var query = $"select Z from Profiles where rowid = @rowid";
        cmd.CommandText = query;
        cmd.Parameters.AddWithValue("@rowid", (int)(y / yRes));
        return cmd;
      }

      float[] Read(SqliteDataReader reader)
      {
        return SelectZDecoder(ZEncoding)((byte[])reader[0],ZResolution);
      }
      
      var z = DBReadQuerySingle(db, CmdBuilder, Read);
      return (false,z[x % z.Length]);

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
    public static async Task<R> DBReadQuerySingleAsync<R>(string name, Func<SqliteCommand, SqliteCommand> cmdBuild, Func<SqliteDataReader, R> rows, CancellationToken stop = default)
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
      if (!File.Exists(db))
      {
        yield return Enumerable.Empty<float>().ToArray();
      }

      SqliteCommand CmdEncoding(SqliteCommand cmd)
      {
        var query = $"select ZEncoding, ZResolution from Meta join Gocator limit 1";
        cmd.CommandText = query;
        return cmd;
      }

      (Int32,float) ReadEncoding(SqliteDataReader reader)
      {
        return (reader.GetInt32(0),reader.GetFloat(1));
      }
      
      var (ZEncoding,ZResolution) = await DBReadQuerySingleAsync(db, CmdEncoding, ReadEncoding, stop);
      
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
        return SelectZDecoder(ZEncoding)((byte[])reader[0],ZResolution);
      }

      var rows = DBReadQuery(db, CmdBuilder, Read, stop);

      await foreach (var r in rows)
      {
        yield return r;
      }
    }

    public static ScanLimits? GetScanLimits(string db)
    {
      if (!File.Exists(db))
      {
        return null;
      }

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

      return DBReadQuerySingle(db, CmdBuilder, Read);
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