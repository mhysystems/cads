using System;
using System.Linq;
using System.Collections.Generic;
using Microsoft.Data.Sqlite;
using System.Runtime.InteropServices;
using System.Diagnostics;
using System.Data;
using System.Threading.Tasks;

using SQLitePCL;
using static SQLitePCL.raw;



namespace cads_gui.Data
{

  public record Profile<T>(double Y, T[] Z);
  public record Profile(double y, double x_off, byte[] z);


  public static class NoAsp
  {

    public static byte[] f2b(float[] z)
    {
      var b = new byte[z.Length * sizeof(float)];
      Buffer.BlockCopy(z, 0, b, 0, b.Length);
      return b;
    }

    public static float[] b2f(byte[] z)
    {
      var b = new float[z.Length / sizeof(float)];
      Buffer.BlockCopy(z, 0, b, 0, z.Length);
      return b;
    }

    public static double profile_width(Profile p, double x_resolution)
    {
      return p.x_off + p.z.Length * x_resolution / sizeof(float);
    }

    public static (double, float[]) make_same_widthf(List<Profile> frame, double x_resolution)
    {

      double x_min = double.MaxValue, x_max = double.MinValue;

      foreach (var p in frame)
      {
        x_min = Math.Min(x_min, p.x_off);
        x_max = Math.Max(x_max, profile_width(p, x_resolution));
      }
      int size = (int)Math.Round((x_max - x_min) / x_resolution);
      float[] ret = new float[size * frame.Count];
      Array.Fill(ret, float.NaN);

      int i = 0;
      foreach (var p in frame)
      {
        int offset = (int)((p.x_off - x_min) / x_resolution);
        Buffer.BlockCopy(p.z, 0, ret, (i + offset) * sizeof(float), p.z.Length);
        i += size;
      }

      return (x_min, ret);
    }

    public static (double, float[]) make_same_widthf2(List<Profile> frame, double x_resolution)
    {

      double x_min = double.MaxValue, x_max = double.MinValue;

      foreach (var p in frame)
      {
        x_min = Math.Min(x_min, p.x_off);
        x_max = Math.Max(x_max, profile_width(p, x_resolution));
      }
      int size = (int)Math.Round((x_max - x_min) / x_resolution);
      float[] ret = new float[size * frame.Count];
      Array.Fill(ret, float.NaN);

      int i = 0;
      foreach (var p in frame)
      {
        int offset = (int)((p.x_off - x_min) / x_resolution);
        Buffer.BlockCopy(p.z, 0, ret, (i + offset) * sizeof(float), p.z.Length);
        i += size;
      }

      return (x_min, ret);
    }

    public static async Task<List<Profile>> RetrieveFrameAsync(string db, double y_min, long len)
    {

      var frame = new List<Profile>();
      var order = len >= 0 ? "asc" : "desc";
      var op = len >= 0 ? ">=" : "<";
      var abslen = Math.Abs(len);

      using var connection = new SqliteConnection("" +
        new SqliteConnectionStringBuilder
        {
          Mode = SqliteOpenMode.ReadOnly,
          DataSource = db
        });

      await connection.OpenAsync();


      var query = $"select * from PROFILE where y {op} @y_min order by y {order} limit @len";
      var command = connection.CreateCommand();
      command.CommandText = query;
      command.Parameters.AddWithValue("@y_min", y_min);
      command.Parameters.AddWithValue("@len", abslen);

      using var reader = command.ExecuteReader();


      while (reader.Read())
      {
        var y = reader.GetDouble(0);
        var x_off = reader.GetDouble(1);
        byte[] z = (byte[])reader[2];

        if (len >= 0)
        {
          frame.Add(new Profile(y, x_off, z));
        }
        else
        {
          frame.Insert(0, new Profile(y, x_off, z));
        }
      }

      return frame;
    }

    public static async Task<List<Profile>> RetrieveFrameModular(string db, double y_min, long len, long left)
    {

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
      commandRes.CommandText = $"select y from profile where rowid = 1 limit 1";

      using var readerRes = commandRes.ExecuteReader(CommandBehavior.SingleRow);
      
      readerRes.Read();
      var y_res = readerRes.GetDouble(0);
      
      readerRes.Close();

      var commandMax = connection.CreateCommand();
      commandMax.CommandText = $"select max(rowid)+1 from profile ";

      using var readerMax = commandMax.ExecuteReader(CommandBehavior.SingleRow);
      
      readerMax.Read();
      var max = readerMax.GetInt64(0);
      
      readerMax.Close();

      var row = (long)Math.Floor(y_min / y_res);
      var rowBegin = Mod(row - left,max);
      var mask = rowBegin > row - left || Mod(row - left + len, max) < row - left + len ? 1.0 : 0.0;
      

      var query = $"select (rowid - @mask*@rowmax)*@yres,x_off,z from PROFILE where rowid >= @row union all select rowid*@yres,x_off,z from profile limit @len";
      var command = connection.CreateCommand();
      command.CommandText = query;
      command.Parameters.AddWithValue("@row", rowBegin);
      command.Parameters.AddWithValue("@len", abslen);
      command.Parameters.AddWithValue("@mask",mask);
      command.Parameters.AddWithValue("@rowmax",max);
      command.Parameters.AddWithValue("@yres",y_res);


      using var reader = command.ExecuteReader();


      while (reader.Read())
      {
        var y = reader.GetDouble(0);
        var x_off = reader.GetDouble(1);
        byte[] z = (byte[])reader[2];

        if (len >= 0)
        {
          frame.Add(new Profile(y, x_off, z));
        }
        else
        {
          frame.Insert(0, new Profile(y, x_off, z));
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

    public static (bool,float) RetrievePoint(string db, double y, long x)
    {
      if (!File.Exists(db)) {
        return (true,0.0f);
      }
      
      using var connection = new SqliteConnection("" +
        new SqliteConnectionStringBuilder
        {
          Mode = SqliteOpenMode.ReadOnly,
          DataSource = db,
        });

      if(connection is null) {
        return (true,0.0f);
      }

      connection.Open();
            
      var command = connection.CreateCommand();      
      
      if(command is null) {
        return (true,0.0f);
      }

      command.CommandText = $"select y from profile where y > 0 limit 1";

      using var reader = command.ExecuteReader(CommandBehavior.SingleRow);
      var y_res =  0.0;
      
      if(reader.Read()) {
        y_res = reader.GetDouble(0);
      }else {
        return (true,0.0f);
      }
      reader.Close();

      command.CommandText = $"select z from profile where rowid = @rowid";
      command.Parameters.AddWithValue("@rowid", (int)(y / y_res));

      using var reader2 = command.ExecuteReader(CommandBehavior.SingleRow);
      
      if(reader2 is null) {
        return (true,0.0f);
      }

      if(reader2.Read()) {

        byte[] z = (byte[])reader2[0];
        var j = Convert(z);
        var r = j[x];
        return (false,r);
      }else {
        return (true,0.0f);
      }

    }
        
    public static string EndpointToSQliteDbName(string site, string belt, DateTime chronos)
    {
      return site + '-' + belt + '-' + chronos.ToString("yyyy-MM-dd-HHmms");
    }

    public static (string site, string belt, DateTime chronos) DecontructSQliteDbName(string filename)
    {
      var p = filename.Split("-");
      var site = p[0];
      var belt = p[1];
      var c = string.Join("-",p[2..^1]);
      var chrono = DateTime.ParseExact(c,"yyyy-MM-dd-HHmms",System.Globalization.CultureInfo.InvariantCulture);
      
      return (site,belt,chrono);
    }

    public static string GetConveyorID(string site, string belt)
    {

      return site + belt;
    }

  } // End class NoAsp

} // End namespace