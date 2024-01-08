using System;
using System.IO;
using Microsoft.AspNetCore.Mvc;
using System.Text.Json;
using System.Threading.Tasks;
using System.Linq;
using Microsoft.AspNetCore.SignalR;

using Google.FlatBuffers;
using CadsFlatbuffers;
using System.Reflection;

namespace cads_gui.Data
{

  [ApiController]
  public class EndpointController : ControllerBase
  {
    private readonly CadsBackend beltservice;

    public EndpointController(CadsBackend helper)
    {
      beltservice = helper;
    }

    [Route("/api/files/{filePath}")]
    [HttpGet]
    public IActionResult GetFile(string filePath)
    {
      var filename = Path.GetFileName(filePath);
      var scanpath = Path.GetFullPath(Path.Combine(beltservice.config.DBPath,filename));
      FileStream destination = new FileStream(scanpath,FileMode.Open, FileAccess.Read);
      return new FileStreamResult(destination, "application/octet-stream")
      {
          FileDownloadName = filename,
          EnableRangeProcessing = true  // this enable the resume abality
      };
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

      var dbPath = beltservice.MakeScanFilePath(site, belt, chrono);
      if(scanData.ContentsType == scan_tables.Auxiliary) {
        var conveyor = scanData.ContentsAsAuxiliary().Conveyor;
        var beltf = scanData.ContentsAsAuxiliary().Belt;
        var limits = scanData.ContentsAsAuxiliary().Limits;
        var gocator = scanData.ContentsAsAuxiliary().Gocator;
        var meta = scanData.ContentsAsAuxiliary().Meta;
        
        if(conveyor is not null && beltf is not null && limits is not null && meta is not null && gocator is not null) {
          ScanData.Insert(dbPath, conveyor.Value);
          ScanData.Insert(dbPath, beltf.Value);
          ScanData.Insert(dbPath, limits.Value);
          ScanData.Insert(dbPath, gocator.Value);
          ScanData.Insert(dbPath, meta.Value);

          var conveyorId = await beltservice.AddConveyorAsync(NoAsp.FromFlatbuffer(conveyor.Value));
          var beltId = await beltservice.AddBeltAsync(NoAsp.FromFlatbuffer(beltf.Value));
          await beltservice.AddInstallAsync(new BeltInstall{ConveyorId = conveyorId, BeltId = beltId, Chrono = chrono});
        }
      
      }else if(scanData.ContentsType == scan_tables.profile_array) {
        var pa = scanData.ContentsAsprofile_array();
        
        if (pa.Count < 1)
        {
          return Ok();
        }

        using var db = new ProfileData(dbPath);
        for (var cnt = 0UL; cnt < pa.Count; cnt++)
        {
          var pc = pa.Profiles((int)cnt);
          if(pc.HasValue) {
            var p = pc.Value;
            var z = p.GetZArray();
            db.Save(pa.Idx + cnt, p.Y, p.X, z);
          }else {
            ArgumentNullException.ThrowIfNull(pc);
          }
        }
      }else if(scanData.ContentsType == scan_tables.register_scan) {
        var registerScan = scanData.ContentsAsregister_scan();
        await beltservice.AddScanAsync(site,belt,registerScan.BeltSerial,chrono);
      }else {
        return NotFound();
      }
    
      return Ok();
    }


  }

}

