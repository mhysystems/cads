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
    public readonly AppSettings config;

    public BeltService(IDbContextFactory<SQLiteDBContext> db, ILogger<BeltService> logger, IOptions<AppSettings> config)
    {
      this.dBContext = db;
      this._logger = logger;
      this.config = config.Value;
    }

    public IEnumerable<Conveyor> GetConveyors()
    {
      using var context = dBContext.CreateDbContext();
      return context.Conveyors.AsEnumerable();
    }

    public bool IsDoubledSided()
    {
      return config.DoubleSided;
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

    public IEnumerable<Scan> GetScans(string site, string belt)
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

    public IEnumerable<Belt> GetBelt(Scan scan)
    {
      using var context = dBContext.CreateDbContext();
      var data = from a in context.Belts where a.Id == scan.Belt select a;
      
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

    public async Task<long> AddConveyorAsync(Conveyor entry)
    {
      using var context = dBContext.CreateDbContext();
      var q = context.Conveyors.Where(e => e.Name == entry.Name && e.Site == entry.Site);
      
      if(q.Any()) {
        return q.First().Id;
      }else {
        context.Conveyors.Add(entry);
        await context.SaveChangesAsync(); 
        return entry.Id;
      }
      
    }

    public async Task<long> AddBeltsAsync(Belt entry)
    {
      using var context = dBContext.CreateDbContext();
      var q = context.Belts.Where(e => e.Conveyor == entry.Conveyor && e.Installed == entry.Installed);
      
      if(q.Any()) {
        return q.First().Id;
      }else {
        context.Belts.Add(entry);
        await context.SaveChangesAsync(); 
        return entry.Id;
      }
    }

    public async Task<float[]> GetBeltProfileAsync(double y, long num_y_samples, Scan belt)
    {
      var dbpath = Path.GetFullPath(Path.Combine(config.DBPath,belt.name));
      var fs = await NoAsp.RetrieveFrameModular(dbpath, y, num_y_samples,0);
      var z = fs.SelectMany( f => f.z).ToArray();
      return z;
    }

    
    public string AppendPath(string belt) {
      return Path.GetFullPath(Path.Combine(config.DBPath,belt));
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

     public async Task<List<ZDepth>> BeltScanAsync2(ZDepthQueryParameters search, Scan belt, long limit) {
      
      var db = AppendPath(NoAsp.EndpointToSQliteDbName(belt.site,belt.conveyor,belt.chrono));
       
      var dx = belt.x_res;
      var dy = belt.y_res;

      var X = search.Width;
      var Y = search.Length;
      var Z = search.ZMax;
      var P = search.Percentage;

      var xMinIndex = (int)Math.Max(Math.Floor(search.XMin / belt.x_res),0);
      var xMaxIndex = (int)Math.Min(Math.Floor(search.XMax / belt.x_res),belt.WidthN);

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

      var x = await Search.SearchParallelAsync(db,columns,rows,(int)belt.WidthN,(long)belt.YmaxN,limit,xRange, fz, fp);

      foreach (var r in x) {
        req.Add(new (r.Col*dx,r.Row*dy,r.Width*dx,r.Length*dy,r.Percent,new(r.ZMin.X*dx,r.ZMin.Y*dy,r.ZMin.Z)));
      }

      return req;

    }
 
  } // class

} // namespace
