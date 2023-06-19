using NATS.Client;
using Microsoft.EntityFrameworkCore;
using Microsoft.Extensions.Options;
using Google.FlatBuffers;
using CadsFlatbuffers;

namespace caas_gui.Data;

public class MsgPublishService
{
    private readonly IDbContextFactory<PostgresDBContext> _dBContext;
    private readonly IOptions<AppSettings> _config;
    public MsgPublishService(IDbContextFactory<PostgresDBContext> dBContext, IOptions<AppSettings> config)
    {
      _dBContext = dBContext;
      _config = config;
    }
    
    (long,bool) SerialToLong(string serial) {

    var valid = long.TryParse(serial, out long aslong);
    return (aslong,valid);
  }
    public void PublishStart(string serial)
    {
      var (serialaslong,valid) = SerialToLong(serial);
      var opts = ConnectionFactory.GetDefaultOptions();
      opts.Url = _config.Value.NatsUrl;
      
      using var c = new ConnectionFactory().CreateConnection(opts);
      using var context = _dBContext.CreateDbContext();
      var row = context.Devices.Where(r => r.Serial == serialaslong).First();
      
      if(valid && row is Device dt) {
        var builder = new FlatBufferBuilder(4096);
        
        var LuaCode = builder.CreateString(row.LuaCode);
        var start = Start.CreateStart(builder,LuaCode);
        var data = CadsFlatbuffers.Msg.CreateMsg(builder,MsgContents.Start,start.Value);
        builder.Finish(data.Value);

        c.Publish(row.MsgSubject, builder.SizedByteArray());
      }
    }
}