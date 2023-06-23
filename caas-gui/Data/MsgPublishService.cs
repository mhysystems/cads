using NATS.Client;
using Microsoft.EntityFrameworkCore;
using Microsoft.Extensions.Options;
using Google.FlatBuffers;
using CadsFlatbuffers;
using Caas;

namespace caas_gui.Data;

public class MsgPublishService
{
  private readonly IDbContextFactory<PostgresDBContext> _dBContext;
  private readonly AppSettings _config;
  public MsgPublishService(IDbContextFactory<PostgresDBContext> dBContext, IOptions<AppSettings> config)
  {
    _dBContext = dBContext;
    _config = config.Value;
  }

  static (int, bool) SerialToInt(string serialCipherText, string key)
  {
    var serial = Caas.KeyGen.Decrypt(serialCipherText,key);
    var valid = int.TryParse(serial, out int asInt);
    return (asInt, valid);
  }

  public (int,bool) GetDevice(string serialCipherText) {
    var (id,valid) = SerialToInt(serialCipherText,_config.UrlKey);
    
    if(!valid) return (0,false);

    using var context = _dBContext.CreateDbContext();
    var row = context?.Devices?.Where(r => r.Serial == id);

    if(row is null || !row.Any()) return (0,false);

    return (id,true);
  }

  public void PublishStart(int serial)
  {
    var opts = ConnectionFactory.GetDefaultOptions();
    opts.Url = _config.NatsUrl;

    using var c = new ConnectionFactory().CreateConnection(opts);
    using var context = _dBContext.CreateDbContext();
    var row = context?.Devices?.Where(r => r.Serial == serial)?.First();

    if (row is not null)
    {
      var builder = new FlatBufferBuilder(4096);

      var LuaCode = builder.CreateString(row.LuaCode);
      var start = Start.CreateStart(builder, LuaCode);
      var data = CadsFlatbuffers.Msg.CreateMsg(builder, MsgContents.Start, start.Value);
      builder.Finish(data.Value);

      c.Publish(row.MsgSubject, builder.SizedByteArray());
    }
  }
}