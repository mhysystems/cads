using Microsoft.AspNetCore.SignalR;
using Microsoft.Extensions.Options;
using System.Text.Json;

using NATS.Client;


using cads_gui.Data;

namespace cads_gui.Services;

public class NatsConsumerHostedService : BackgroundService
{
  protected readonly ILogger<NatsConsumerHostedService> _logger;
  protected readonly IHubContext<RealtimeHub> _realtimehubContext;

  protected readonly AppSettings _config;


  public NatsConsumerHostedService(ILogger<NatsConsumerHostedService> logger, IOptions<AppSettings> config, IHubContext<RealtimeHub> realtimehubContext)
  {
    _logger = logger;
    _realtimehubContext = realtimehubContext;
    _config = config.Value;
  }

  protected async override Task ExecuteAsync(CancellationToken stoppingToken)
  {
    try {
    Object testLock = new Object();
    var args = ConnectionFactory.GetDefaultOptions();
    args.Url = _config.NatsUrl;
    using var c = new ConnectionFactory().CreateConnection(args);
    EventHandler<MsgHandlerEventArgs> msgHandler = (sender, args) =>
 {

   if (args.Message.Subject == "/realtime")
   {
     var json = JsonSerializer.Deserialize<Realtime>(new ReadOnlySpan<byte>(args.Message.Data));
     _realtimehubContext.Clients.Group("/realtime"+json.Site+json.Conveyor).SendAsync("ReceiveMessageRealtime", json);
   }
   else if (args.Message.Subject == "/realtimemeta")
   {

     var json = JsonSerializer.Deserialize<MetaRealtime>(new ReadOnlySpan<byte>(args.Message.Data));
     _realtimehubContext.Clients.Group("/realtimemeta"+json.Site+json.Conveyor).SendAsync("ReceiveMessageMeta", json);
   }

   if (false)
   {

     lock (testLock)
     {
       Monitor.Pulse(testLock);
     }
   }
 };
    using IAsyncSubscription s = c.SubscribeAsync(">", msgHandler);
    while (!stoppingToken.IsCancellationRequested)
    {

      await Task.Delay(10000, stoppingToken);
    }
    lock (testLock)
    {
      Monitor.Wait(testLock);
    }
    }catch(NATSConnectionException e) {
      _logger.LogError("Unable to connect to Nats server. Realtime infomation disabled. Nats error: {} ",e.Message);
    }
  }

  public async Task StopAsync(CancellationToken cancellationToken)
  {
    //  await _subscription?.DrainAsync();
    //  _subscription?.Unsubscribe();
  }
}