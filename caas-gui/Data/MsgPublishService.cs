using NATS.Client;
using Microsoft.EntityFrameworkCore;
using Microsoft.Extensions.Options;
using Google.FlatBuffers;
using CadsFlatbuffers;
using Caas;
using System.Security.Authentication;

namespace caas_gui.Data;

public class MsgPublishService
{
  private readonly IDbContextFactory<CaasDBContext> _dBContext;
  private readonly AppSettings _config;
  public MsgPublishService(IDbContextFactory<CaasDBContext> dBContext, IOptions<AppSettings> config)
  {
    _dBContext = dBContext;
    _config = config.Value;
  }

  static int? SerialToInt(string serialCipherText, string key)
  {
    var serial = Caas.KeyGen.Decrypt(serialCipherText,key);
    var valid = int.TryParse(serial, out int asInt);
    return valid ? asInt : null;
  }

  public Device? GetDevice(string serialCipherText) {
    
    int? id = _config.DeviceSerial;

    if(!String.IsNullOrEmpty(serialCipherText)) {
    
      id = _config.Obfuscate switch {
        true  => SerialToInt(serialCipherText,_config.UrlKey),
        _     => Int32.Parse(serialCipherText)
      };
    
    }
    
    if(!id.HasValue) return null;

    using var context = _dBContext.CreateDbContext();
    var row = context?.Devices?.Where(r => r.Serial == id);

    if(row is null || !row.Any()) return null;

    return row?.First();
  }

  public IEnumerable<Conveyor> GetConveyers(Device device)
  {
    using var context = _dBContext.CreateDbContext();

    var row = context?.Conveyors?.Where (r => r.Org == device.Org);

    return row?.ToArray() ?? Enumerable.Empty<Conveyor>();

  }

  public void PublishLuaCode(string subject, string luaCode) 
  {
    var opts = ConnectionFactory.GetDefaultOptions();
    opts.Url = _config.NatsUrl;

    using var c = new ConnectionFactory().CreateConnection(opts);
    var builder = new FlatBufferBuilder(4096);

    var LuaCode = builder.CreateString(luaCode);
    var start = Start.CreateStart(builder, LuaCode);
    var data = CadsFlatbuffers.Msg.CreateMsg(builder, MsgContents.Start, start.Value);
    builder.Finish(data.Value);

    c.Publish(subject, builder.SizedByteArray());
  }

  public void PublishStart(Device device, Conveyor conveyor)
  {
    var opts = ConnectionFactory.GetDefaultOptions();
    opts.Url = _config.NatsUrl;

    using var c = new ConnectionFactory().CreateConnection(opts);

    var builder = new FlatBufferBuilder(4096);

    var LuaCode = builder.CreateString(conveyor.LuaCode);
    var start = Start.CreateStart(builder, LuaCode);
    var data = CadsFlatbuffers.Msg.CreateMsg(builder, MsgContents.Start, start.Value);
    builder.Finish(data.Value);

    c.Publish(device.MsgSubjectPublish, builder.SizedByteArray());
    Db.UpdateDeviceStatus(_dBContext,device);
  }

  public void PublishAlign(Device device) 
  {
    var luaCode = File.ReadAllText(_config.AlignmentCode);
    PublishLuaCode(device.MsgSubjectPublish, luaCode);
    Db.UpdateDeviceStatus(_dBContext,device);
  }

  public void PublishStop(Device device)
  {
    var opts = ConnectionFactory.GetDefaultOptions();
    opts.Url = _config.NatsUrl;

    using var c = new ConnectionFactory().CreateConnection(opts);

    var builder = new FlatBufferBuilder(4096);

    Stop.StartStop(builder);
    var stop = Stop.EndStop(builder);
    var data = CadsFlatbuffers.Msg.CreateMsg(builder, MsgContents.Stop, stop.Value);
    builder.Finish(data.Value);

    c.Publish(device.MsgSubjectPublish, builder.SizedByteArray());
    Db.UpdateDeviceStatus(_dBContext,device);
  }
}