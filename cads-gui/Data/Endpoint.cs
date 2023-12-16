using System;
using System.IO;
using Microsoft.AspNetCore.Mvc;
using System.Text.Json;
using System.Threading.Tasks;
using System.Linq;
using Microsoft.AspNetCore.SignalR;

using Google.FlatBuffers;
using CadsFlatbuffers;

namespace cads_gui.Data
{

  [ApiController]
  public class EndpointController : ControllerBase
  {
    private readonly BeltService beltservice;

    public EndpointController(BeltService helper)
    {
      beltservice = helper;
    }

    // localhost:5000/Endpoint
    [Route("/api/belt/{belt}/{y:double}/{len:long}/{left:long?}")]
    [HttpGet]
    public async Task<IActionResult> Get(string belt, double y, long len, long left = 0)
    {

      try
      {
        if (len < 1) return NotFound();

        var dbpath = Path.GetFullPath(Path.Combine(beltservice.config.DBPath,belt));
        var frame = await NoAsp.RetrieveFrameModular(dbpath, y, len+1, left+1);
        var z = frame.Skip(1).SkipLast(1).SelectMany(x => x.z).ToArray();
        var builder = new FlatBufferBuilder(frame.Capacity);

        var data = CadsFlatbuffers.plot_data.Createplot_data(
          builder,
          CadsFlatbuffers.plot_data.CreateYSamplesVector(builder, frame.Skip(1).SkipLast(1).Select(x => x.y).ToArray()),
          CadsFlatbuffers.plot_data.CreateZSamplesVector(builder, z),
          frame.Last().y,
          frame.First().y
        );

        builder.Finish(data.Value);

        var ms = new MemoryStream(builder.SizedByteArray());
        return new FileStreamResult(ms, "application/octet-stream");

      }
      catch (Exception)
      {
        return NotFound();
      }
    }

    [Route("/api/meta")]
    [HttpPost]
    public async Task<IActionResult> Post_meta([FromBody] Scan json)
    {
      await beltservice.StoreBeltConstantsAsync(json);
      return Ok();
    }

    [Route("/api/belt/{site}/{belt}/{chrono}")]
    [HttpPost]
    public async Task<IActionResult> Post_profile_bulk(string site, string belt, DateTime chrono)
    {
      using var ms = new MemoryStream();

      await Request.Body.CopyToAsync(ms);
      var jjd = ms.ToArray();
      var bb = new ByteBuffer(jjd);
      var pa = profile_array.GetRootAsprofile_array(bb);

      if (pa.Count < 1)
      {
        return Ok();
      }

      ulong cnt = 0;
      var (zmaxInit, err) = beltservice.SelectBeltPropertyZmax(site, belt, chrono);
      double zmax = err == 0 ? zmaxInit : 0;

      var dbname = Path.Combine(beltservice.config.DBPath,NoAsp.EndpointToSQliteDbName(site, belt, chrono));
      using var db = new ProfileData(dbname);

      while (cnt++ < pa.Count)
      {
        var pc = pa.Profiles((int)cnt - 1);
        if(pc.HasValue) {
          var p = pc.Value;
          var z = p.GetZSamplesArray();
          zmax = Math.Max(zmax, z.Max());
          db.Save(pa.Idx + cnt - 1, p.Y, p.XOff, z);
        }else {
          ArgumentNullException.ThrowIfNull(pc);
        }
      }

      if (zmax > zmaxInit)
      {
        await beltservice.UpdateBeltPropertyZmax(site, belt, chrono, zmax);
      }

      return Ok();

    }
  
    [Route("/api/scan/{site}/{belt}/{chrono}")]
    [HttpPost]
    public async Task<IActionResult> PostScan(string site, string belt, DateTime chrono)
    {
      using var ms = new MemoryStream();
      await Request.Body.CopyToAsync(ms);
      
      var bb = new ByteBuffer(ms.ToArray());
      
      var scanData = scan.GetRootAsscan(bb);

      var dbname = Path.Combine(beltservice.config.DBPath,NoAsp.EndpointToSQliteDbName(site, belt, chrono));
      if(scanData.ContentsType == scan_tables.conveyor) {
        ScanData.Insert(dbname, scanData.ContentsAsconveyor());
      }else if(scanData.ContentsType == scan_tables.belt) {
        ScanData.Insert(dbname,scanData.ContentsAsbelt());
      }else if(scanData.ContentsType == scan_tables.profile_array) {
        var pa = scanData.ContentsAsprofile_array();
        
        if (pa.Count < 1)
        {
          return Ok();
        }

        using var db = new ProfileData(dbname);
        for (var cnt = 0UL; cnt < pa.Count; cnt++)
        {
          var pc = pa.Profiles((int)cnt);
          if(pc.HasValue) {
            var p = pc.Value;
            var z = p.GetZSamplesArray();
            db.Save(pa.Idx + cnt, p.Y, p.XOff, z);
          }else {
            ArgumentNullException.ThrowIfNull(pc);
          }
        }
      }else {
        return NotFound();
      }
    
      return Ok();
    }


  }

}

