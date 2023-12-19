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


    [Route("/api/scan/{site}/{belt}/{chrono}")]
    [HttpPost]
    public async Task<IActionResult> PostScan(string site, string belt, DateTime chrono)
    {
      using var ms = new MemoryStream();
      await Request.Body.CopyToAsync(ms);
      
      var bb = new ByteBuffer(ms.ToArray());
      
      var scanData = scan.GetRootAsscan(bb);

      var dbname = NoAsp.MakeScanFilename(site, belt, chrono);
      if(scanData.ContentsType == scan_tables.conveyor) {
        ScanData.Insert(dbname, scanData.ContentsAsconveyor());
        await beltservice.AddConveyorAsync(NoAsp.FromFlatbuffer(scanData.ContentsAsconveyor()));
      }else if(scanData.ContentsType == scan_tables.belt) {
        ScanData.Insert(dbname,scanData.ContentsAsbelt());
        await beltservice.AddBeltAsync(NoAsp.FromFlatbuffer(scanData.ContentsAsbelt()));
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

