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

using cads_gui.BeltN;

namespace cads_gui.Data
{
  public record P3(double x, double y, double z);

  // ZDepth represents an area with origin x and y 
  // t is total z samples on belt found less than a certain number
  // z in the minimum z sample found in area before total threshold
  public record ZDepth(double x, double y, double width, double length, double t, P3 z);

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

    public IEnumerable<Conveyor> GetConveyors()
    {
      using var context = dBContext.CreateDbContext();
      return context.Conveyors.AsEnumerable();
    }

    public bool IsDoubledSided()
    {
      return _config.DoubleSided;
    }

    public IEnumerable<Conveyor> GetConveyors(string site)
    {
      using var context = dBContext.CreateDbContext();
      return from row in context.Conveyors where row.Site == site select row;
    }

    public IEnumerable<string> GetConveyorsString(string site)
    {
      using var context = dBContext.CreateDbContext();
      var data = from row in context.Scans where row.site == site select row.conveyor;

      return data.Distinct().ToList();
    }

    public async Task<(double, double, double)> GetBeltBoundary(string belt)
    {
      return await NoAsp.BeltBoundaryAsync(belt);
    }

    public IEnumerable<Scan> GetBelts(string site, string belt)
    {
      using var context = dBContext.CreateDbContext();
      var data = from a in context.Scans orderby a.chrono where a.site == site && a.conveyor == belt select a;
      
      return data.ToList();
    }

    public List<Scan> GetBelts(string site, string belt, DateTime chrono)
    {
      using var context = dBContext.CreateDbContext();
      var data = from a in context.Scans orderby a.chrono where a.site == site && a.conveyor == belt && a.chrono.Date == chrono.Date select a;
      
      return data.ToList();
    }

    public IEnumerable<Scan> GetBeltsAsync(string site, string conveyor)
    {
      using var context = dBContext.CreateDbContext();
      var data = from a in context.Scans orderby a.chrono where a.site == site && a.conveyor == conveyor select a;
      
      // context will be disposed without evaluation of data
      return data.ToArray(); 

    }

    public async Task<IEnumerable<DateTime>> GetBeltDatesAsync(Scan belt)
    {
      using var context = dBContext.CreateDbContext();
      return await Task.Run(() => (from a in context.Scans orderby a.chrono where a.site == belt.site && a.conveyor == belt.conveyor select a.chrono).ToArray());
    }

    public async Task UpdateBeltPropertyYmax(string site, string belt, DateTime chrono, double ymax, long ymaxn)
    {
      using var context = dBContext.CreateDbContext();
      var data = from a in context.Scans where a.site == site && a.conveyor == belt && a.chrono == chrono select a;

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
      var data = from a in context.Scans where a.site == site && a.conveyor == belt && a.chrono == chrono select a;

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
      var data = from a in context.Scans where a.site == site && a.conveyor == belt && a.chrono == chrono select a;

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
      var data = from a in context.Scans where a.site == site && a.conveyor == belt && a.chrono == chrono select a;

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

    public IEnumerable<(string, string)> GetSitesWithCads()
    {
      using var context = dBContext.CreateDbContext();
      // EFcore returns IQueryable which doesn't handle selecting tuples or order by very well, so convert to Enumerable
      var rows = (from a in context.Scans orderby a.chrono select a).ToList();
      var sites = from r in rows group r by r.site into site select (site.Key, site.First().conveyor);
      return sites;
    }


    public async Task StoreBeltConstantsAsync(Scan entry)
    {
      using var context = dBContext.CreateDbContext();
      DateTime.SpecifyKind(entry.chrono,DateTimeKind.Utc);
      context.Scans.Add(entry);
      await context.SaveChangesAsync();
    }

    public async Task<int> AddConveyorAsync(Conveyor entry)
    {
      using var context = dBContext.CreateDbContext();
      var q = context.Conveyors.Where(e => e.Name == entry.Name && e.Site == entry.Site && e.Installed == entry.Installed);
      
      if(q.Any()) {
        return q.First().Id;
      }else {
        context.Conveyors.Add(entry);
        await context.SaveChangesAsync(); 
        return entry.Id;
      }
      
    }

    public async Task<(double, float[])> GetBeltProfileAsync(double y, long num_y_samples, Scan belt)
    {
      var dbpath = Path.GetFullPath(Path.Combine(_config.DBPath,belt.name));
      var fs = await NoAsp.RetrieveFrameModular(dbpath, y, num_y_samples);
      return NoAsp.make_same_widthf(fs, belt.x_res);
    }

    
    public string AppendPath(string belt) {
      return Path.GetFullPath(Path.Combine(_config.DBPath,belt));
    }

    
    public List<SavedZDepthParams> GetSavedZDepthParams(Scan belt)
    {
      using var context = dBContext.CreateDbContext();
      var rtn = from a in context.SavedZDepthParams where a.Conveyor == belt.conveyor && a.Site == belt.site select a;
      return rtn.ToList();
    }

    public void AddDamage(SavedZDepthParams entry)
    {
      using var context = dBContext.CreateDbContext();
      var q = context.SavedZDepthParams.Where(e => e.Name == entry.Name && e.Site == entry.Site && e.Conveyor == entry.Conveyor);
      if(q.Any()) {
        context.SavedZDepthParams.Update(entry);
      }else{
        context.SavedZDepthParams.Add(entry);
      }
      context.SaveChanges();

    }

    
    /// <summary>
    /// Search for areas of interest on a section of belt.
    /// </summary>
    /// <param name="X"> Length(mm) of area in X direction</param>
    /// <param name="Y"> Length(mm) of area in Y direction</param>
    /// <param name="p"> Percentage of points in area given true by fn</param>
    /// <param name="fn"> Boolean function with z height as input.</param>
    /// <returns></returns>
    public async IAsyncEnumerable<(List<ZDepth>, P3)> BeltScanAsync(double X, double Y, double p, double Z, Scan BeltConstants, long offset, long y_len)
    {

      bool fn(float z) {
        return z < Z;
      }
      const double cut_y = 9.0;

      var dx = BeltConstants.x_res;
      var dy = BeltConstants.y_res;

      var area_sample_x = (long)Math.Ceiling(X / dx);
      var area_sample_y = (long)Math.Ceiling(Y / dy);

      var zy = y_len;

      (double x_start, float[] zs) = await GetBeltProfileAsync(offset, y_len, BeltConstants);
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

        long x;
        var x_coord = new List<ZDepth>();
        var zmin = new P3(0, 0, Double.MaxValue);

        for (x = 0; (x + area_sample_x) <= zx; x += area_sample_x)
        {
          var (total, pz) = sub_area(x, y, area_sample_x, ry);
          zmin = zmin.z < pz.z ? zmin : pz;

          if (total >= p)
          {
            x_coord.Add(new ZDepth(x * dx + x_start, (y + offset) * dy, area_sample_x * dx, area_sample_y * dy, (long)(total*100), pz));
          }
        }

        if (x < zx)
        {
          long rx = zx - x;
          var (total, pz) = sub_area(x, y, rx, ry);
          zmin = zmin.z < pz.z ? zmin : pz;

          if (total >= p)
          {
            x_coord.Add(new ZDepth(x * dx + x_start, (y + offset) * dy, area_sample_x * dx, area_sample_y * dy, (long)(total*100), pz));
          }
        }

        return (x_coord, zmin);
      };

      long y;
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

     public async Task<List<ZDepth>> BeltScanAsync2(ZDepthQueryParameters search, Scan belt, long limit) {
      
      var db = AppendPath(NoAsp.EndpointToSQliteDbName(belt.site,belt.conveyor,belt.chrono));
      var xOff = - belt.Xmax / 2;
      
      var dx = belt.x_res;
      var dy = belt.y_res;

      var X = search.Width;
      var Y = search.Length;
      var Z = search.Depth;
      var P = search.Percentage;

      var xMinIndex = (int)Math.Max(Math.Floor((search.XMin - xOff) / belt.x_res),0);
      var xMaxIndex = (int)Math.Min(Math.Floor((search.XMax - xOff) / belt.x_res),belt.WidthN);

      var columns = (int)Math.Ceiling(belt.Xmax / X);
      var rows = (int)Math.Ceiling(belt.Ymax / Y);

      bool fz(float z) 
      {
        return z < Z;
      }

      bool fp(Search.SearchResult p) 
      {
        return p.Percent > P;
      }

      bool xRange(int xIndex) {
        return xMinIndex <= xIndex && xIndex < xMaxIndex;
      }

      var req = new List<ZDepth>();
  
      foreach (var r in await Search.SearchParallelAsync(db,columns,rows,(int)belt.WidthN,(long)belt.YmaxN,limit,xRange, fz, fp)) {
        req.Add(new (r.Col*dx + xOff,r.Row*dy,r.Width*dx,r.Length*dy,r.Percent,new(r.ZMin.X*dx + xOff,r.ZMin.Y*dy,r.ZMin.Z)));
      }

      return req;

    }
 

    public async Task<double> GetLength(string belt)
    {
      var (min, max, _) = await GetBeltBoundary(belt);
      return max - min;
    }



  } // class

} // namespace
