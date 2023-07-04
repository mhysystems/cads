using Microsoft.AspNetCore.SignalR;
using Microsoft.Extensions.Options;
using System.Text.Json;
using Microsoft.EntityFrameworkCore;

using NATS.Client;
using caas_gui.Data;

namespace caas_gui.Services;

public class NatsConsumerHostedService : BackgroundService
{
  protected readonly ILogger<NatsConsumerHostedService> _logger;
  protected readonly IHubContext<MessagesHub> _messageshubContext;
  protected readonly AppSettings _config;
  protected readonly IDbContextFactory<PostgresDBContext> _dBContext;

  public NatsConsumerHostedService(IWebHostEnvironment env
    ,ILogger<NatsConsumerHostedService> logger
    ,IOptions<AppSettings> config
    ,IHubContext<MessagesHub> messageshubContext
    ,IDbContextFactory<PostgresDBContext> dBContext
)
  {
    _logger = logger;
    _messageshubContext = messageshubContext;
    _config = config.Value;
    _dBContext = dBContext;
  }

  protected (Device,string)? ValidateSubject(string? subject) {
    
    if(string.IsNullOrEmpty(subject)) return null;
    
    var path = subject.Split(".");

    if(path.Length != 3) return null;

    var caas = path[0];
    var serialStr = path[1];
    var id = path[2];

    if(caas != "caas") return null;

    if(!Int32.TryParse(serialStr,out int serial)) return null;
    
    var device = Db.GetDevice(_dBContext,serial);

    if(device is null) return null;

    return (device,id);
  }

  protected async override Task ExecuteAsync(CancellationToken stoppingToken)
  {
    while (!stoppingToken.IsCancellationRequested)
    {
      try
      {
        
        var args = ConnectionFactory.GetDefaultOptions();
        args.Url = _config.NatsUrl;
        using var c = new ConnectionFactory().CreateConnection(args) ?? throw new NullReferenceException("Nats CreateConnection is null");

        async void msgHandler(object? sender, MsgHandlerEventArgs args)
        {
          var options = new JsonSerializerOptions
          {
            PropertyNameCaseInsensitive = true
          };
          
          var subject = ValidateSubject(args.Message?.Subject);
          
          if(subject is not null) {

            var (device,id) = subject.Value;

            if(id == "heartbeat") {
              using var context = _dBContext.CreateDbContext();

              context?.Devices?.Where( r => r.Serial == device.Serial)?.ExecuteUpdate ( r =>
                r.SetProperty( d => d.LastSeen,DateTime.Now)
              );
            
            }
          }else {
            _logger.LogError("Nats message missing subject");
          }

        };

        using var caas = c.SubscribeAsync("caas.*", msgHandler) ?? throw new NullReferenceException("Nats subscribe is null");

        while (!stoppingToken.IsCancellationRequested)
        {
          _logger.LogInformation("Connected to Nats and waiting for messages");
          await Task.Delay(new TimeSpan(0,1,0), stoppingToken);

          using var context = _dBContext.CreateDbContext();

          if(context is not null) {
            var now = DateTime.Now;
            foreach(var device in context.Devices) {
              if(device.LastSeen.AddMinutes(1) < now && device.State == DeviceState.Connected) {
                device.State = DeviceState.Disconnect;
                await _messageshubContext.Clients.Group(device.Serial.ToString()).SendAsync("UpdateDevice",device,stoppingToken);
              }
            }
            context.SaveChanges();
          }
          
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