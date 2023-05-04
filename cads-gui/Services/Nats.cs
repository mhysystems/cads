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
    while (true)
    {
      try
      {
        var args = ConnectionFactory.GetDefaultOptions();
        args.Url = _config.NatsUrl;
        using var c = new ConnectionFactory().CreateConnection(args);
        EventHandler<MsgHandlerEventArgs> msgHandler = (sender, args) =>
        {

          if (args.Message.Subject == "/realtime")
          {
            var msg = new ReadOnlySpan<byte>(args.Message.Data);
            var json = JsonSerializer.Deserialize<Realtime>(msg);
            if (json is not null)
            {
              _realtimehubContext.Clients.Group("/realtime" + json.Site + json.Conveyor).SendAsync("ReceiveMessageRealtime", json);
            }
          }
          else if (args.Message.Subject == "/realtimemeta")
          {

            var json = JsonSerializer.Deserialize<MetaRealtime>(new ReadOnlySpan<byte>(args.Message.Data));
            if (json is not null)
            {
              _realtimehubContext.Clients.Group("/realtimemeta" + json.Site + json.Conveyor).SendAsync("ReceiveMessageMeta", json);
            }
          }

        };

        using IAsyncSubscription s = c.SubscribeAsync(">", msgHandler);
        while (!stoppingToken.IsCancellationRequested)
        {
          _logger.LogInformation("Connected to Nats and waiting for messages");
          await Task.Delay(Timeout.Infinite, stoppingToken);
        }

        _logger.LogInformation("Leaving Nats background service");
        break;

      }
      catch (NATSConnectionException e)
      {
        _logger.LogError("Unable to connect to Nats server. Realtime infomation disabled. Nats error: {} ", e.Message);
        await Task.Delay(60000, stoppingToken);
      }
    }
  }

}