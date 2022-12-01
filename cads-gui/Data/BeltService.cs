using System;
using System.Linq;
using System.Threading.Tasks;
using System.IO;
using System.Collections.Generic;
using Microsoft.Data.Sqlite;
using System.Diagnostics;
using System.Text.Json;
using Microsoft.Extensions.Logging;
using Microsoft.EntityFrameworkCore;
using Microsoft.Extensions.Options;

using Microsoft.CodeAnalysis.CSharp.Scripting;
using Microsoft.CodeAnalysis.Scripting;

using Microsoft.AspNetCore.SignalR;

using System.Reactive;
using System.Reactive.Linq;

using SQLitePCL;
using static SQLitePCL.raw;

namespace cads_gui.Data
{
  public record P3(double x, double y, double z);

  // ZDepth represents an area with origin x and y 
  // t is total z samples on belt found less than a certain number
  // z in the minimum z sample found in area before total threshold
  public record ZDepth(double x, double y, double width, double length, long t, P3 z);

  public class BeltService
  {
    private readonly IDbContextFactory<SQLiteDBContext> dBContext;
    private readonly ILogger<BeltService> _logger;
    public readonly AppSettings _config;

    public BeltService(IDbContextFactory<SQLiteDBContext> db, ILogger<BeltService> logger, IOptions<AppSettings> config)
    {
      this.dBContext = db;
      this._logger = logger;
      this._config = config.Value;
    }

    public IEnumerable<Conveyors> GetConveyors()
    {
      using var context = dBContext.CreateDbContext();
      return context.Conveyors.AsEnumerable();
    }

    public IEnumerable<Conveyors> GetConveyors(string site)
    {
      using var context = dBContext.CreateDbContext();
      return from row in context.Conveyors where row.Site == site select row;
    }

    public async Task<(double, double, double)> GetBeltBoundary(string belt)
    {
      return await NoAsp.BeltBoundaryAsync(belt);
    }

    public Task<IQueryable<BeltChart>> GetBeltChartAsync(Func<BeltOld, Boolean> w = null)
    {

      w = w ?? (_ => true);
      using var context = dBContext.CreateDbContext();
      var t = from a in context.anomalies.Where(w).OrderBy(row => row.z_depth).ThenByDescending(row => row.length)//.Take(64)
              join b in context.chart on a.anomaly_ID equals b.anomaly_ID
              select new BeltChart(a, b.visible);

      return Task.FromResult(t.AsQueryable());

    }

    public Belt GetBelt(string site, string belt)
    {
      using var context = dBContext.CreateDbContext();
      var data = from a in context.belt orderby a.chrono where a.site == site && a.conveyor == belt select a;
      
      if (data.Count() > 1)
        return data.Last();
      else
        return data.First();
    }

    public Belt GetBelt(string site, string belt, DateTime chrono)
    {
      using var context = dBContext.CreateDbContext();
      var data = from a in context.belt orderby a.chrono where a.site == site && a.conveyor == belt && a.chrono.Date == chrono.Date select a;
      
      if (data.Count() > 1)
        return data.Last();
      else if (data.Count() == 1)
        return data.First();
      else
        return null;
    }

    public Task<List<Belt>> GetBeltsAsync(string site, string conveyor)
    {
      using var context = dBContext.CreateDbContext();
      var data = from a in context.belt orderby a.chrono where a.site == site && a.conveyor == conveyor select a;
      
      return data.ToListAsync();

    }

    public async Task<IEnumerable<DateTime>> GetBeltDatesAsync(Belt belt)
    {
      using var context = dBContext.CreateDbContext();
      return await Task.Run(() => (from a in context.belt orderby a.chrono where a.site == belt.site && a.conveyor == belt.conveyor select a.chrono).ToArray());
    }

    public async Task UpdateBeltPropertyYmax(string site, string belt, DateTime chrono, double ymax, long ymaxn)
    {
      using var context = dBContext.CreateDbContext();
      var data = from a in context.belt where a.site == site && a.conveyor == belt && a.chrono == chrono select a;

      if (data.Any())
      {
        var row = data.First();
        row.Ymax = ymax;
        row.YmaxN = ymaxn;
        await context.SaveChangesAsync();
      }
    }

    public async Task UpdateBeltPropertyWidthN(string site, string belt, DateTime chrono, double param)
    {
      using var context = dBContext.CreateDbContext();
      var data = from a in context.belt where a.site == site && a.conveyor == belt && a.chrono == chrono select a;

      if (data.Any())
      {
        var row = data.First();
        row.WidthN = param;
        await context.SaveChangesAsync();
      }
    }

    public async Task UpdateBeltPropertyZmax(string site, string belt, DateTime chrono, double param)
    {
      using var context = dBContext.CreateDbContext();
      var data = from a in context.belt where a.site == site && a.conveyor == belt && a.chrono == chrono select a;

      if (data.Any())
      {
        var row = data.First();
        row.z_max = param;
        await context.SaveChangesAsync();
      }
    }

    public (double, int) SelectBeltPropertyZmax(string site, string belt, DateTime chrono)
    {
      using var context = dBContext.CreateDbContext();
      var data = from a in context.belt where a.site == site && a.conveyor == belt && a.chrono == chrono select a;

      if (data.Any())
      {
        var row = data.First();
        return (row.z_max, 0);
      }
      else
      {
        return (0, 1);
      }
    }

    public IEnumerable<(string, string)> GetSites()
    {
      using var context = dBContext.CreateDbContext();
      // EFcore returns IQueryable which doesn't handle selecting tuples or order by very well, so convert to Enumerable
      var rows = (from a in context.belt orderby a.chrono select a).ToList();
      return from r in rows group r by r.site into site select (site.Key, site.First().conveyor);
    }


    public async Task StoreBeltConstantsAsync(Belt entry)
    {
      using var context = dBContext.CreateDbContext();
      DateTime.SpecifyKind(entry.chrono,DateTimeKind.Utc);
      context.belt.Add(entry);
      await context.SaveChangesAsync();
    }

    public async Task<(double, float[])> GetBeltProfileAsync(double y, long num_y_samples, Belt belt)
    {
      var dbpath = Path.GetFullPath(Path.Combine(_config.DBPath,belt.name));
      var fs = await NoAsp.RetrieveFrameModular(dbpath, y, num_y_samples);
      return NoAsp.make_same_widthf(fs, belt.x_res);
    }

    
    public async Task<(DateTime,float)[]> ConveyorsHeightAsync(double y, long x, IEnumerable<Belt> belts) {
			
      var dbg = belts.Select(x => (x.chrono,Path.GetFullPath(Path.Combine(_config.DBPath,x.name)))).Take(30);
      
      return await NoAsp.ConveyorsHeightAsync(dbg, y, x, _logger);

    }

    public void SetBeltAsync(BeltOld b)
    {
      using var context = dBContext.CreateDbContext();
      context.anomalies.Update(b);
      context.SaveChanges();
    }

    public void SetChartAsync(Chart b)
    {
      using var context = dBContext.CreateDbContext();
      var r = context.chart.Find(b.anomaly_ID);
      r.visible = b.visible;
      //dBContext.CreateDbContext().chart.Update(b);
      context.SaveChanges();
    }

    public List<SavedZDepthParams> GetSavedZDepthParams(Belt belt)
    {
      using var context = dBContext.CreateDbContext();
      var rtn = from a in context.SavedZDepthParams where a.Conveyor == belt.conveyor && a.Site == belt.site select a;
      return rtn.ToList();
    }

    public void AddDamage(SavedZDepthParams d)
    {
      using var context = dBContext.CreateDbContext();
      context.SavedZDepthParams.Add(d);
      context.SaveChanges();

    }


    public async Task<List<(double, string)>> get_color_range(string s, Belt BeltConstants, double cmin = Double.MinValue)
    {


      _logger.LogDebug($"get_color_range({s})");


      List<(double, string)> colour_range = new List<(double, string)>();

      var sw = Stopwatch.StartNew();
      using JsonDocument doc = JsonDocument.Parse("{" + s + "}");
      JsonElement root = doc.RootElement;
      var sum = root.GetProperty("sum").GetDouble();
      var area = root.GetProperty("area").GetDouble();
      var query = $"z => {root.GetProperty("query").GetString()}";


      Func<float, bool> fn;

      try
      {
        var options = ScriptOptions.Default.AddReferences(typeof(cads_gui.Data.BeltOld).Assembly);
        fn = await CSharpScript.EvaluateAsync<Func<float, bool>>(query, options);

        float z_max = (float)BeltConstants.z_max;
        float z_min = 0f;
        var steps = 1000;

        double red_acc = 25.0;
        double red_scale = 1.0;
        Func<float, bool, double> red_colour_range = (float z, bool b) =>
        {
          red_acc += red_scale * (b ? 1 : 0);
          return red_acc;
        };

        double grey_acc = 0.0;
        double grey_scale = 1.0;
        Func<float, bool, double> grey_colour_range = (float z, bool b) =>
        {
          if (z >= cmin) grey_acc += grey_scale * (b ? 0 : 1);
          return grey_acc;
        };

        double red_acc_tmp = 0.0;
        double grey_acc_tmp = 0.0;
        for (int n = 0; n < steps; n++)
        {
          var z = (z_max - z_min) * n / steps + z_min;
          var b = fn(z);
          red_acc_tmp = red_colour_range(z, b);
          grey_acc_tmp = grey_colour_range(z, b);
        }

        red_acc = 25.0;
        red_scale = 50 / (red_acc_tmp - red_acc);
        grey_acc = 0.0;
        grey_scale = 100 / grey_acc_tmp;

        var r1 = red_colour_range(z_min, fn(z_min));
        var g1 = grey_colour_range(z_min, fn(z_min));
        bool is_red0 = fn(z_min);
        bool is_grey0 = z_min >= cmin;
        colour_range.Add((0, "hsl(240,100,50)"));
        colour_range.Add(is_red0 ? (0.002, "hsl(0,100,25)") : (0.002, "hsl(0,0,100)"));

        for (int n = 1; n < steps - 1; n++)
        {
          var z = (z_max - z_min) * n / steps + z_min;
          var c_index = n / (double)steps;
          var is_red1 = fn(z);
          var is_grey1 = z >= cmin;
          var r = red_colour_range(z, is_red1);
          var g = grey_colour_range(z, is_red1);

          if (is_red0 != is_red1)
          {
            if (is_red1)
            {
              colour_range.Add((c_index, $"hsl(0,0,{100 - (int)g})"));
              colour_range.Add((c_index, $"hsl(0,100,{(int)r})"));
            }
            else
            {
              colour_range.Add((c_index, $"hsl(0,100,{(int)r})"));
              colour_range.Add((c_index, $"hsl(0,0,{100 - (int)g})"));
            }

          }

          if (!is_red1 && is_grey0 != is_grey1)
          {

            colour_range.Add((c_index, $"hsl(0,0,{100 - (int)g})"));

          }

          is_red0 = is_red1;
          is_grey0 = is_grey1;
        }

        is_red0 = fn(z_max);
        colour_range.Add(is_red0 ? (1.0, "hsl(0,100,75)") : (1.0, "hsl(0,0,0)"));

      }
      catch (CompilationErrorException)
      {

      }


      _logger.LogDebug($"colour_range running time in ms: {sw.ElapsedMilliseconds}");

      return colour_range;
    }

    public async IAsyncEnumerable<(List<ZDepth>, P3)> BeltScanAsync(string s, Belt BeltConstants, long offset, long y_len)
    {

      var sw = Stopwatch.StartNew();
      using JsonDocument doc = JsonDocument.Parse("{" + s + "}");
      JsonElement root = doc.RootElement;
      var sum = root.GetProperty("sum").GetDouble();
      var area = root.GetProperty("area").GetDouble();
      var query = $"z => {root.GetProperty("query").GetString()}";


      Func<float, bool> fn = (float x) => false;

      try
      {
        var options = ScriptOptions.Default.AddReferences(typeof(cads_gui.Data.BeltOld).Assembly);
        fn = await CSharpScript.EvaluateAsync<Func<float, bool>>(query, options);
      }
      catch (CompilationErrorException)
      {
        _logger.LogDebug($"CompilationErrorException");
      }

      var seg = Math.Sqrt(area); // Assume area is a square      

      await foreach (var e in BeltScanAsync(seg, seg, sum, fn, BeltConstants, offset, y_len))
      {
        yield return e;
      }


    }



    /// <summary>
    /// Search for areas of interest on a section of belt.
    /// </summary>
    /// <param name="X"> Length(mm) of area in X direction</param>
    /// <param name="Y"> Length(mm) of area in Y direction</param>
    /// <param name="p"> Percentage of points in area given true by fn</param>
    /// <param name="fn"> Boolean function with z height as input.</param>
    /// <returns></returns>
    public async IAsyncEnumerable<(List<ZDepth>, P3)> BeltScanAsync(double X, double Y, double p, double Z, Belt BeltConstants, long offset, long y_len)
    {

      bool fn(float z) {
        return z < Z;
      }
      const double cut_y = 9.0;

      var dx = BeltConstants.x_res;
      var dy = BeltConstants.y_res;

      var area_sample_x = (long)Math.Ceiling(X / dx);
      var area_sample_y = (long)Math.Ceiling(Y / dy);

      var sum = (long)Math.Ceiling(p * area_sample_x * area_sample_y);
      var zy = y_len;

      var (x_start, zs) = await GetBeltProfileAsync(offset, y_len, BeltConstants);
      var zx = zs.Length / zy;
      area_sample_x = area_sample_x > zx ? zx : area_sample_x;


      (double, P3) sub_area(long x, long y, long x_max, long y_max)
      {

        double total = 0;


        var zmin = new P3(0.0, 0.0, Double.MaxValue);

        for (long j = 0; j < y_max; j++)
        {
          for (long i = 0; i < x_max; i++)
          {
            var z = zs[(y + j) * zx + i + x];
            if (!float.IsNaN(z) && z > cut_y) zmin = zmin.z < z ? zmin : new P3((i + x) * dx + x_start, (offset + j + y) * dy, z);

            if (!float.IsNaN(z) && z > cut_y && fn(z))
            {
              total++;
            }
          }
        }

        return (total / (y_max * x_max), zmin);

      };


      (List<ZDepth>, P3) shift_x(long y, long ry)
      {

        long x = 0;
        var x_coord = new List<ZDepth>();
        var zmin = new P3(0, 0, Double.MaxValue);

        for (x = 0; (x + area_sample_x) <= zx; x += area_sample_x)
        {
          var (total, pz) = sub_area(x, y, area_sample_x, ry);
          zmin = zmin.z < pz.z ? zmin : pz;

          if (total >= p)
          {
            x_coord.Add(new ZDepth(x * dx + x_start, (y + offset) * dy, area_sample_x * dx, area_sample_y * dy, (long)total, pz));
          }
        }

        if (x < zx)
        {
          long rx = zx - x;
          var (total, pz) = sub_area(x, y, rx, ry);
          zmin = zmin.z < pz.z ? zmin : pz;

          if (total >= p)
          {
            x_coord.Add(new ZDepth(x * dx + x_start, (y + offset) * dy, area_sample_x * dx, area_sample_y * dy, (long)total, pz));
          }
        }

        return (x_coord, zmin);
      };

      long y = 0;
      var r = new List<ZDepth>();
      var zmin = new P3(0, 0, Double.MaxValue);

      for (y = 0; (y + area_sample_y) <= zy; y += area_sample_y)
      {
        var (xs, pz) = shift_x(y, area_sample_y);
        zmin = zmin.z < pz.z ? zmin : pz;
        r.AddRange(xs);
      }

      if (y < zy)
      {
        long ry = zy - y;
        var (xs, pz) = shift_x(y, ry);
        zmin = zmin.z < pz.z ? zmin : pz;
        r.AddRange(xs);
      }

      yield return (r, zmin); //Useless but will leave for now

    }




    /// <summary>
    /// Search for areas of interest on a section of belt.
    /// </summary>
    /// <param name="X"> Length(mm) of area in X direction</param>
    /// <param name="Y"> Length(mm) of area in Y direction</param>
    /// <param name="p"> Percentage of points in area given true by fn</param>
    /// <param name="fn"> Boolean function with z height as input.</param>
    /// <returns></returns>
    public async IAsyncEnumerable<(List<ZDepth>, P3)> BeltScanAsync(double X, double Y, double p, Func<float, bool> fn, Belt BeltConstants, long offset, long y_len)
    {

      
      const double cut_y = 9.0;

      var dx = BeltConstants.x_res;
      var dy = BeltConstants.y_res;

      var area_sample_x = (long)Math.Ceiling(X / dx);
      var area_sample_y = (long)Math.Ceiling(Y / dy);

      var sum = (long)Math.Ceiling(p * area_sample_x * area_sample_y);
      var zy = y_len;

      var (x_start, zs) = await GetBeltProfileAsync(offset, y_len, BeltConstants);
      var zx = zs.Length / zy;
      area_sample_x = area_sample_x > zx ? zx : area_sample_x;


      (long, P3) sub_area(long x, long y, long x_max, long y_max)
      {

        long total = 0;


        var zmin = new P3(0.0, 0.0, Double.MaxValue);

        for (long j = 0; j < y_max; j++)
        {
          for (long i = 0; i < x_max; i++)
          {
            var z = zs[(y + j) * zx + i + x];
            if (!float.IsNaN(z) && z > cut_y) zmin = zmin.z < z ? zmin : new P3((i + x) * dx + x_start, (j + y) * dy, z);

            if (!float.IsNaN(z) && z > cut_y && fn(z))
            {
              total++;
            }
          }
        }

        return (total, zmin);

      };


      (List<ZDepth>, P3) shift_x(long y, long ry)
      {

        long x = 0;
        var x_coord = new List<ZDepth>();
        var zmin = new P3(0, 0, Double.MaxValue);

        for (x = 0; (x + area_sample_x) <= zx; x += area_sample_x)
        {
          var (total, pz) = sub_area(x, y, area_sample_x, ry);
          zmin = zmin.z < pz.z ? zmin : pz;

          if (total >= sum)
          {
            x_coord.Add(new ZDepth(x * dx + x_start, (y + offset) * dy, area_sample_x * dx, area_sample_y * dy, total, pz));
          }
        }

        if (x < zx)
        {
          long rx = zx - x;
          var (total, pz) = sub_area(x, y, rx, ry);
          zmin = zmin.z < pz.z ? zmin : pz;

          if (total >= sum)
          {
            x_coord.Add(new ZDepth(x * dx + x_start, (y + offset) * dy, area_sample_x * dx, area_sample_y * dy, total, pz));
          }
        }

        return (x_coord, zmin);
      };

      long y = 0;
      var r = new List<ZDepth>();
      var zmin = new P3(0, 0, Double.MaxValue);

      for (y = 0; (y + area_sample_y) <= zy; y += area_sample_y)
      {
        var (xs, pz) = shift_x(y, area_sample_y);
        zmin = zmin.z < pz.z ? zmin : pz;
        r.AddRange(xs);
      }

      if (y < zy)
      {
        long ry = zy - y;
        var (xs, pz) = shift_x(y, ry);
        zmin = zmin.z < pz.z ? zmin : pz;
        r.AddRange(xs);
      }

      yield return (r, zmin); //Useless but will leave for now

    }

    public async Task<List<(double, string)>> GetColorRangeAsync(Func<float, bool> fn, Belt BeltConstants, double cmin = Double.MinValue)
    {


      _logger.LogDebug($"get_color_range2()");


      var colour_range = new List<(double, string)>();

      cmin = 0;

      var sw = Stopwatch.StartNew();

      try
      {
        float z_max = (float)BeltConstants.z_max;
        float z_min = 0f;
        var steps = 1000;

        double red_acc = 25.0;
        double red_scale = 1.0;
        Func<float, bool, double> red_colour_range = (float z, bool b) =>
        {
          red_acc += red_scale * (b ? 1 : 0);
          return red_acc;
        };

        double grey_acc = 0.0;
        double grey_scale = 1.0;
        Func<float, bool, double> grey_colour_range = (float z, bool b) =>
        {
          if (z >= cmin) grey_acc += grey_scale * (b ? 0 : 1);
          return grey_acc;
        };

        double red_acc_tmp = 0.0;
        double grey_acc_tmp = 0.0;
        for (int n = 0; n < steps; n++)
        {
          var z = (z_max - z_min) * n / steps + z_min;
          var b = fn(z);
          red_acc_tmp = red_colour_range(z, b);
          grey_acc_tmp = grey_colour_range(z, b);
        }

        red_acc = 25.0;
        red_scale = 50 / (red_acc_tmp - red_acc);
        grey_acc = 0.0;
        grey_scale = 100 / grey_acc_tmp;

        var r1 = red_colour_range(z_min, fn(z_min));
        var g1 = grey_colour_range(z_min, fn(z_min));
        bool is_red0 = fn(z_min);
        bool is_grey0 = z_min >= cmin;
        colour_range.Add((0, "hsl(240,100,50)"));
        colour_range.Add(is_red0 ? (0.002, "hsl(0,100,25)") : (0.002, "hsl(0,0,100)"));

        for (int n = 1; n < steps - 1; n++)
        {
          var z = (z_max - z_min) * n / steps + z_min;
          var c_index = n / (double)steps;
          var is_red1 = fn(z);
          var is_grey1 = z >= cmin;
          var r = red_colour_range(z, is_red1);
          var g = grey_colour_range(z, is_red1);

          if (is_red0 != is_red1)
          {
            if (is_red1)
            {
              colour_range.Add((c_index, $"hsl(0,0,{100 - (int)g})"));
              colour_range.Add((c_index, $"hsl(0,100,{(int)r})"));
            }
            else
            {
              colour_range.Add((c_index, $"hsl(0,100,{(int)r})"));
              colour_range.Add((c_index, $"hsl(0,0,{100 - (int)g})"));
            }

          }

          if (!is_red1 && is_grey0 != is_grey1)
          {

            colour_range.Add((c_index, $"hsl(0,0,{100 - (int)g})"));

          }

          is_red0 = is_red1;
          is_grey0 = is_grey1;
        }

        is_red0 = fn(z_max);
        colour_range.Add(is_red0 ? (1.0, "hsl(0,100,75)") : (1.0, "hsl(0,0,0)"));

      }
      catch (CompilationErrorException)
      {

      }


      _logger.LogDebug($"colour_range running time in ms: {sw.ElapsedMilliseconds}");
      await Task.CompletedTask;
      return colour_range;
    }

    public async Task<double> GetLength(string belt)
    {
      var (min, max, _) = await GetBeltBoundary(belt);
      return max - min;
    }



  } // class

} // namespace
