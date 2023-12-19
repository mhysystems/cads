using Microsoft.EntityFrameworkCore;
using Microsoft.Extensions.Options;
using System.Reactive.Linq;
using InfluxDB.Client;
using InfluxDB.Client.Api.Domain;
using InfluxDB.Client.Core;

using cads_gui.BeltN;
using System.Security.Cryptography;
using CadsFlatbuffers;
using System.Data;

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
    private readonly IWebHostEnvironment _env;

    public BeltService(IDbContextFactory<SQLiteDBContext> db, ILogger<BeltService> logger, IOptions<AppSettings> config, IWebHostEnvironment env)
    {
      _env = env;
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


    public async Task<List<(string,double)>> GetLastMeasures(string site, string conveyor)
    {

      var bucket = _env.EnvironmentName;
      var results = new List<(string,double)>();

      try
      {
        using var client = new InfluxDBClient(config.InfluxDB, config.InfluxAuth);

        var query = $"""
        from(bucket: "{bucket}")
        |> range(start: -1d)
        |> filter(fn: (r) =>  r._measurement == "beltrotationperiod" or 
                              r._measurement == "pulleyspeed" or 
                              r._measurement == "beltlength" or
                              r._measurement == "cadstoorigin" or
                              r._measurement == "pulleyoscillation")
        |> filter(fn :(r) => r.conveyor == "{conveyor}" and r.site == "{site}")
        |> last()
      """;

        var fluxTables = await client.GetQueryApi().QueryAsync(query, "MHY");
        
        foreach (var data in fluxTables)
        {
          var records = data.Records;
          foreach (var rec in records)
          {
            results.Add((rec.GetMeasurement(),(double)rec.GetValue()));
          }
        }

      }
      catch (Exception e)
      {
        _logger.LogError("{}", e.Message);
      }

      return results;

    }

    public async Task<List<AnomalyMsg>> GetAnomalies(string site, string conveyor)
    {
      var bucket = _env.EnvironmentName;
      var timezone = GetConveyorTimezone(site, conveyor);
      var results = new List<AnomalyMsg>();

      try
      {
        using var client = new InfluxDBClient(config.InfluxDB, config.InfluxAuth);

        var query = $$"""
        import "timezone"
        import "date"

        lastm = from(bucket: "{{bucket}}")
          |> range(start: 0)
          |> filter(fn: (r) => r["_measurement"] == "anomaly")
          |> filter(fn: (r) => r["_field"] == "location" or r["_field"] == "value")
          |> filter(fn: (r) => r["conveyor"] == "{{conveyor}}")
          |> filter(fn: (r) => r["site"] == "{{site}}")
          |> last()
          |> findColumn(fn: (key) => true, column: "_time")
        if length(arr:lastm) > 0 then
        from(bucket: "{{bucket}}")
          |> range(start: date.add(to:lastm[0], d:-30m), stop: date.add(to:lastm[0], d:1s))
          |> filter(fn: (r) => r["_measurement"] == "anomaly")
          |> filter(fn: (r) => r["_field"] == "location" or r["_field"] == "value")
          |> filter(fn: (r) => r["conveyor"] == "{{conveyor}}")
          |> filter(fn: (r) => r["site"] == "{{site}}")
          |> map(fn: (r) => ({r with _time: date.time(t: r._time, location : timezone.location(name: "{{timezone.Id}}"))}))
          |> pivot(rowKey: ["_time"], columnKey: ["_field"], valueColumn: "_value")
          |> limit(n:64)
          |> sort(columns: ["_time"], desc: true)
        else
        from(bucket: "{{bucket}}")
          |> range(start: 0, stop: 1)
          |> filter(fn: (r) => false)
      """;

        var fluxTables = await client.GetQueryApi().QueryAsync(query, "MHY");

        foreach (var data in fluxTables)
        {
          var records = data.Records;
          foreach (var rec in records)
          {
            var a = new AnomalyMsg
            {
              Measurement = rec.GetMeasurement(),
              Site = (string)rec.GetValueByKey("site"),
              Conveyor = (string)rec.GetValueByKey("conveyor"),
              Quality = 0,
              Revision = Convert.ToInt32(rec.GetValueByKey("revison")),
              Value = (double)rec.GetValueByKey("value"),
              Location = (double)rec.GetValueByKey("location"),
              Timestamp = rec.GetTimeInDateTime().GetValueOrDefault(DateTime.Now)
            };

            results.Add(a);
          }
        }
      }
      catch (Exception e)
      {
        _logger.LogError("{}", e.Message);
      }

      return results;
    }

    public IEnumerable<string> GetConveyorsString(string site)
    {
      using var context = dBContext.CreateDbContext();
      var data = from row in context.Conveyors where row.Site == site select row.Name;

      return data.Distinct().ToList();
    }

    public TimeZoneInfo GetConveyorTimezone(string site, string conveyor)
    {
      using var context = dBContext.CreateDbContext();
      var data = from a in context.Conveyors where a.Site == site && a.Name == conveyor select a;

      return data.Any() ? data.First().Timezone : TimeZoneInfo.Local;
    }

    public IEnumerable<Scan> GetScans(string site, string conveyorName)
    {
      using var context = dBContext.CreateDbContext();
      var data = from scan in context.Scans 
        join conveyor in context.Conveyors on scan.ConveyorId equals conveyor.Id 
        orderby scan.Chrono 
        where conveyor.Site == site && conveyor.Name == conveyorName select scan;

      return data.ToList();
    }

    public List<Scan> GetScans(string site, string conveyorName, DateTime chrono)
    {
      using var context = dBContext.CreateDbContext();
      var data = from scan in context.Scans 
        join conveyor in context.Conveyors on scan.ConveyorId equals conveyor.Id 
        orderby scan.Chrono 
        where conveyor.Site == site && conveyor.Name == conveyorName && scan.Chrono == chrono select scan;

      return data.ToList();
    }


    public async Task<Belt> GetBelt(string site, string conveyorName)
    {
      using var context = await dBContext.CreateDbContextAsync();
      var rows = from installs in context.BeltInstalls 
        join conveyor in context.Conveyors on installs.ConveyorId equals conveyor.Id
        join belt in context.Belts on installs.BeltId equals belt.Id 
        where conveyor.Site == site && conveyor.Name == conveyorName
        orderby installs.Chrono 
        select belt;

      return rows.First();
    }

    public Belt GetBelt(Scan scan)
    {
      using var context = dBContext.CreateDbContext();
      var data = from belt in context.Belts where belt.Id == scan.BeltId select belt;

      return data.First();
    }

    public Conveyor? GetConveyor(Scan scan)
    {
      using var context = dBContext.CreateDbContext();
      var data = from conveyor in context.Conveyors where conveyor.Id == scan.ConveyorId select conveyor;

      return data?.First();
    }

    public IEnumerable<Belt> GetBelts(Scan scan)
    {
      using var context = dBContext.CreateDbContext();
      var data = from belt in context.Belts where belt.Id == scan.BeltId select belt;

      return data.ToArray();
    }

    public async Task<IEnumerable<DateTime>> GetBeltDatesAsync(Scan scan)
    {
      using var context = dBContext.CreateDbContext();
      return await Task.Run(() => (from s in context.Scans orderby s.Chrono where s.ConveyorId == scan.ConveyorId && s.BeltId == scan.BeltId select s.Chrono).ToArray());
    }


    public IEnumerable<(string, string)> GetSitesWithCads()
    {
      using var context = dBContext.CreateDbContext();
      
      // EFcore returns IQueryable which doesn't handle selecting tuples or order by very well, so convert to List
      var conveyors = (from conveyor in context.Conveyors select conveyor).ToList();
      
      // tuple (map.Key,map.First().Name) is (conveyor site name, [conveyor].First().Name)
      var sites = from conveyor in conveyors group conveyor by conveyor.Site into map select (map.Key, map.First().Name);
      return sites;
    }
    public IEnumerable<Conveyor> GetConveyors(string site)
    {
      using var context = dBContext.CreateDbContext();
      return from row in context.Conveyors where row.Site == site select row;
    }

    public async Task<long> AddConveyorAsync(Conveyor entry)
    {
      using var context = dBContext.CreateDbContext();
      var q = context.Conveyors.Where(e => e.Name == entry.Name && e.Site == entry.Site);

      if (q.Any())
      {
        return q.First().Id;
      }
      else
      {
        context.Conveyors.Add(entry);
        await context.SaveChangesAsync();
        return entry.Id;
      }

    }

    public async Task<long> AddBeltAsync(Belt entry)
    {
      
      using var context = dBContext.CreateDbContext();
      var q = context.Belts.Where(e => e.Serial == entry.Serial);

      if (q.Any())
      {
        return q.First().Id;
      }
      else
      {
        context.Belts.Add(entry);
        await context.SaveChangesAsync();
        return entry.Id;
      }
    }

    public Belt? GetBeltAsync(string serial)
    {
      
      using var context = dBContext.CreateDbContext();
      var q = context.Belts.Where(e => e.Serial == serial);

      if (q.Any())
      {
        return null;
      }
      else
      {
        return q.First();
      }
    }

    public async Task AddScanAsync(Scan scan)
    {
      
      using var context = dBContext.CreateDbContext();
      
      var isInstalled = from install in context.BeltInstalls 
        where install.ConveyorId == scan.ConveyorId && install.BeltId == scan.BeltId
        select install;

      if(isInstalled.Any())
      {
        DateTime.SpecifyKind(scan.Chrono, DateTimeKind.Utc);
        context.Scans.Add(scan);
        await context.SaveChangesAsync();
      }
    }

    public async Task<IEnumerable<Grafana>> GetGrafanaPlotsAsync(Belt belt)
    {
      using var context = await dBContext.CreateDbContextAsync();
      var beltplots = from b in context.Plots where b.Belt == belt.Id && b.Visible select b;

      return beltplots.ToList();
    }

    public ScanLimits? GetScanLimits(Scan scan)
    {
      return NoAsp.GetScanLimits(MakeScanFilePath(scan));
    }

    public async Task<float[]> GetBeltProfileAsync(double y, long num_y_samples, Scan scan)
    {
      var fs = await NoAsp.RetrieveFrameModular(MakeScanFilePath(scan), y, num_y_samples, 0);
      var z = fs.SelectMany(f => f.z).ToArray();
      return z;
    }

    public string MakeScanFilePath(string site, string conveyor, DateTime chrono)
    {
      var filename = NoAsp.MakeScanFilename(site,conveyor,chrono);
      return Path.GetFullPath(Path.Combine(config.DBPath, filename));
    }

    public string MakeScanFilePath(Scan scan)
    {
      var filename = MakeScanFilename(scan);
      return Path.GetFullPath(Path.Combine(config.DBPath, filename));
    }

    public string MakeScanFilename(Scan scan)
    {
      using var context = dBContext.CreateDbContext();
      var data = from conveyor in context.Conveyors where conveyor.Id == scan.ConveyorId select conveyor;
      if(data.Any()){
        var conveyor = data.First();
        return NoAsp.MakeScanFilename(conveyor.Site,conveyor.Name,scan.Chrono);
      }else {
        return String.Empty;
      }
    }

    public List<SavedZDepthParams> GetSavedZDepthParams(Scan scan)
    {
      using var context = dBContext.CreateDbContext();
      var rtn = from conveyor in context.Conveyors
        join saved in context.SavedZDepthParams on new {Site = conveyor.Site, Conveyor = conveyor.Name} equals new {Site = saved.Site, Conveyor = saved.Conveyor} 
        where conveyor.Id == scan.ConveyorId
        select saved;
      return rtn.ToList();
    }

    public void AddDamage(SavedZDepthParams entry)
    {
      using var context = dBContext.CreateDbContext();
      var q = context.SavedZDepthParams.Where(e => e.Name == entry.Name && e.Site == entry.Site && e.Conveyor == entry.Conveyor);
      if (q.Any())
      {
        context.SavedZDepthParams.Update(entry);
      }
      else
      {
        context.SavedZDepthParams.Add(entry);
      }
      context.SaveChanges();

    }

    public async Task<List<ZDepth>> BeltScanAsync2(ZDepthQueryParameters search, Scan scan, long limit)
    {

      var belt = GetScanLimits(scan);
      
      if(belt is null) {
        return Enumerable.Empty<ZDepth>().ToList();
      }

      var dx = belt.Width / belt.WidthN;
      var dy = belt.Length / belt.LengthN;

      var X = search.Width;
      var Y = search.Length;
      var ZMax = search.ZMax;
      var ZMin = search.ZMin;
      var P = search.Percentage;

      var xMinIndex = (int)Math.Max(Math.Floor(search.XMin / dx), 0);
      var xMaxIndex = (int)Math.Min(Math.Floor(search.XMax / dx), belt.WidthN);

      var columns = (int)Math.Ceiling(belt.Width / X);
      var rows = (int)Math.Ceiling(belt.Length / Y);

      bool fz(float z)
      {
        return ZMin < z && z < ZMax;
      }

      bool fp(Search.SearchResult p)
      {
        return p.Percent > P;
      }

      bool xRange(int xIndex)
      {
        return xMinIndex <= xIndex && xIndex < xMaxIndex;
      }

      var req = new List<ZDepth>();

      var x = await Search.SearchParallelAsync(scan.Filepath, columns, rows, (int)belt.WidthN, belt.LengthN, limit, xRange, fz, fp);

      foreach (var r in x)
      {
        req.Add(new(r.Col * dx, r.Row * dy, r.Width * dx, r.Length * dy, r.Percent, new(r.ZMin.X * dx, r.ZMin.Y * dy, r.ZMin.Z)));
      }

      return req;

    }

  } // class

} // namespace
