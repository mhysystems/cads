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
      

      var query = $"select (rowid - @mask*@rowmax)*@yres,z from PROFILE where rowid >= @row union all select rowid*@yres,z from profile limit @len";
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
        var z = ConvertBytesToFloats((byte[])reader[1]);

        if (len >= 0)
        {
          frame.Add(new Profile(y,z));
        }
        else
        {
          frame.Insert(0, new Profile(y,z));
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
  } // End class NoAsp

} // End namespace