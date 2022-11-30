using System;
using System.Linq;
using System.Collections.Generic;
using Microsoft.Data.Sqlite;
using System.Runtime.InteropServices;
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
      var ret = new float[size * frame.Count];
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
        
        if(len >=0) {
          frame.Add(new Profile(y, x_off, z));
        }else{
          frame.Insert(0,new Profile(y, x_off, z));
        }
      }

      return frame;
    }

    public static double Mod(double x, double n) { return ((x % n) + n) % n; }
    static float[] Convert(byte[] a) {

      ReadOnlySpan<byte> z_ptr = new(a);
      var j = MemoryMarshal.Cast<byte, float>(z_ptr);
      return j.ToArray();
    
    }

    public static async IAsyncEnumerable<Profile<float>> RetrieveFrameForwardAsync(string db, double y, long len, ILogger logger = null)
    {
      if (File.Exists(db))
      {
        len = Math.Abs(len);

        using var connection = new SqliteConnection("" +
          new SqliteConnectionStringBuilder
          {
            Mode = SqliteOpenMode.ReadOnly,
            DataSource = db
          });

        await connection.OpenAsync();
        var query = $"select * from PROFILE where y >= @y_min order by y limit @len";
        var command = connection.CreateCommand();

        command.CommandText = "select max(y) from PROFILE";
        var y_max = (double)await command.ExecuteScalarAsync();

        y = Mod(y, y_max);
        command.CommandText = query;
        command.Parameters.AddWithValue("@y_min", y);
        command.Parameters.AddWithValue("@len", len);
        command.Prepare();

        while (len > 0)
        {

          using var reader = command.ExecuteReader();

          while (await reader.ReadAsync())
          {
            var y_row = reader.GetDouble(0);
            var x_off = reader.GetDouble(1);
            byte[] z = (byte[])reader[2];
            var j = Convert(z);

            yield return new Profile<float>(y_row, j);
            len--;

          }

          command.Parameters[0].Value = 0;
          command.Parameters[1].Value = len;
        }
      }else {
        logger?.LogError("Belt DB {} file doesn't exits",db);
      }
    }


    public static async IAsyncEnumerable<(DateTime,double)> ConveyorsHeightAsync(IAsyncEnumerable<(DateTime,string)> belts, double y, long x, ILogger logger = null) {
			
      await foreach (var (date,name) in belts) {
        await foreach (var r in RetrieveFrameForwardAsync(name,y,1,logger)) {
          yield return (date,r.Z[x]);
        }  
      }
    }

    public static async IAsyncEnumerable<(DateTime,double)> ConveyorsHeightAsync(IEnumerable<(DateTime,string)> belts, double y, long x, ILogger logger = null) {
			
      foreach (var (date,name) in belts) {
        await foreach (var r in RetrieveFrameForwardAsync(name,y,1,logger)) {
          yield return (date,r.Z[x]);
        }  
      }
    }

    public static async Task<List<Profile>> RetrieveFrameModular(string belt, double y_min, long len, long left)
    {
      var (min, max, cnt) = await BeltBoundaryAsync(belt);
      var beltLength = max + (max - min) / cnt;
      var fst = await RetrieveFrameModular(belt, y_min, -left);
      
      fst.AddRange(await RetrieveFrameModular(belt, y_min, len));

      // Plotly requires y axis to be totally ordered, so need to change y near the belt end to negative numbers around origin
      if(fst.Any()) {
        var first = fst.First().y ;
        if(first > fst.Last().y ) {
          fst = fst.Select(e => {
            if(e.y >= first) {
              return new Profile(e.y - beltLength,e.x_off,e.z);
            }else {
              return e;
            }
          }).ToList();
        }
      }
      return fst;
    }

    public static async Task<List<Profile>> RetrieveFrameModular(string belt, double y_min, long len)
    {
      var (min, max, cnt) = await BeltBoundaryAsync(belt);
      var beltLength = max + (max - min) / cnt;
      var sign = Math.Sign(len);
      var origin = sign > 0 ? min : beltLength;

      len = Math.Abs(len);
      len = Math.Min(len, cnt);

      var real_y_min = Mod(y_min, beltLength);
      var fst = await RetrieveFrameAsync(belt, real_y_min, sign * len);

      if (fst.Count < len)
      {
        if(sign > 0) {
          fst.AddRange(await RetrieveFrameAsync(belt, origin, sign * (len - fst.Count)));
        }else{
          fst.InsertRange(0,await RetrieveFrameAsync(belt, origin, sign * (len - fst.Count)));
        }
      }

      // Plotly requires y axis to be totally ordered, so need to change y near the belt end to negative numbers around origin
      if(fst.Any()) {
        var first = fst.First().y ;
        if(first > fst.Last().y ) {
          fst = fst.Select(e => {
            if(e.y >= first) {
              return new Profile(e.y - beltLength,e.x_off,e.z);
            }else {
              return e;
            }
          }).ToList();
        }
      }
      return fst;
    }

    public static (double, double, long) BeltBoundary(string belt)
    {
      double first = 0.0;
      double last = 0.0;
      long count = 0;

      using var connection = new SqliteConnection("" +
        new SqliteConnectionStringBuilder
        {
          Mode = SqliteOpenMode.ReadOnly,
          DataSource = belt
        });

      connection.Open();
      var command = connection.CreateCommand();

      command.CommandText = @"SELECT MIN(Y),MAX(Y),COUNT(Y) FROM PROFILE";

      using var reader = command.ExecuteReader();

      while (reader.Read())
      {
        first = reader.GetDouble(0);
        last = reader.GetDouble(1);
        count = reader.GetInt64(2);
      }

      connection.Close();

      return (first, last, count);
    }

    public static (double, double, long, long) BeltBoundary2(string belt)
    {
      double first = 0.0;
      double last = 0.0;
      long count = 0;
      long width = 0;

      using var connection = new SqliteConnection("" +
        new SqliteConnectionStringBuilder
        {
          Mode = SqliteOpenMode.ReadOnly,
          DataSource = belt
        });

      connection.Open();
      var command = connection.CreateCommand();

      command.CommandText = @"SELECT MIN(Y),MAX(Y),COUNT(Y),LENGTH(Z) FROM PROFILE LIMIT 1";

      using var reader = command.ExecuteReader();

      while (reader.Read())
      {
        first = reader.GetDouble(0);
        last = reader.GetDouble(1);
        count = reader.GetInt64(2);
        width = reader.GetInt64(3) / sizeof(float);
      }

      connection.Close();

      return (first, last, count, width);
    }

    public static async Task<(double, double, long)> BeltBoundaryAsync(string belt)
    {
      return await Task.Run(() => BeltBoundary(belt));
    }

    public static string EndpointToSQliteDbName(string site, string belt, DateTime chronos)
    {

      return site + '-' + belt + '-' + chronos.ToString("yyyy-MM-dd-HHmms");
    }

    public static string GetConveyorID(string site, string belt)
    {

      return site + belt;
    }

    public static async Task<long> frame_count(string belt)
    {

      var (_, _, length) = await BeltBoundaryAsync(belt);

      return length;
    }
  } // End class NoAsp

} // End namespace