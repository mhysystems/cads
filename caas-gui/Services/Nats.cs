using Microsoft.AspNetCore.SignalR;
using Microsoft.Extensions.Options;
using System.Text.Json;

using NATS.Client;
using caas_gui.Data;

namespace caas_gui.Services;

public class NatsConsumerHostedService : BackgroundService
{
  protected readonly ILogger<NatsConsumerHostedService> _logger;
  protected readonly IHubContext<MessagesHub> _messageshubContext;

  protected readonly AppSettings _config;

  protected readonly IWebHostEnvironment _env;


  public NatsConsumerHostedService(IWebHostEnvironment env, ILogger<NatsConsumerHostedService> logger, IOptions<AppSettings> config, IHubContext<MessagesHub> messageshubContext)
  {
    _env = env;
    _logger = logger;
    _messageshubContext = messageshubContext;
    _config = config.Value;
  }

  protected async override Task ExecuteAsync(CancellationToken stoppingToken)
  {
    while (!stoppingToken.IsCancellationRequested)
    {
      try
      {
        var env = _env.EnvironmentName;
        ArgumentNullException.ThrowIfNull(env);
        
        var args = ConnectionFactory.GetDefaultOptions();
        args.Url = _config.NatsUrl;
        using var c = new ConnectionFactory().CreateConnection(args);

        async void msgHandler(object? sender, MsgHandlerEventArgs args)
        {
          var options = new JsonSerializerOptions
          {
            PropertyNameCaseInsensitive = true
          };
          /*
          if(args.Message.HasHeaders && args.Message.Header["category"] is not null) {
            var category = args.Message.Header["category"];
            if(category == "measure") {
              var json = JsonSerializer.Deserialize<MeasureMsg>(new ReadOnlySpan<byte>(args.Message.Data),options);
              if (json is not null && json.Quality != -1)
              {
                await _messageshubContext.Clients.Group("measurements" + json.Site + json.Conveyor).SendAsync("ReceiveMessage", json,stoppingToken);
              }else {
                _logger.LogError("Nats message unable to be deserialised to JSON. {}",System.Text.Encoding.UTF8.GetString(args.Message.Data));
              }
            
            }else if(category == "anomaly") {
              var json = JsonSerializer.Deserialize<AnomalyMsg>(new ReadOnlySpan<byte>(args.Message.Data),options);
              if (json is not null && json.Quality != -1)
              {
                await _messageshubContext.Clients.Group("anomalies" + json.Site + json.Conveyor).SendAsync("ReceiveMessage", json,stoppingToken);
              }else {
                _logger.LogError("Nats message unable to be deserialised to JSON. {}",System.Text.Encoding.UTF8.GetString(args.Message.Data));
              }
            }else {
              
            }
          }else {
            _logger.LogError("Nats message missing header");
          }
          */
        };

        using IAsyncSubscription measure = c.SubscribeAsync(env, msgHandler);

        while (!stoppingToken.IsCancellationRequested)
        {
          _logger.LogInformation("Connected to Nats and waiting for messages");
          await Task.Delay(Timeout.Infinite, stoppingToken);
        }

        _logger.LogInformation("Leaving Nats background service");

      }
      catch (NATSConnectionException e)
      {
        _logger.LogError("Unable to connect to Nats server. Realtime infomation disabled. Nats error: {} ", e.Message);
        await Task.Delay(60000, stoppingToken);
      }
    }
  }

}