
namespace caas_gui.Data;


[Flags] public enum DeviceState {
   Disconnect = 0,
   Connected = 1,
   Scanning = 2,
   Aligned = 4
}

public enum DBBackend {
  Sqlite,
  Postgres
}

public class AppSettings
{
  public string NatsUrl { get; set; } = "127.0.0.1";

  public DBBackend DBBackend { get; set; } = DBBackend.Postgres;
  public string PostgresSqlConnectionString { get; set; } = String.Empty;
  public string SqliteConnectionString { get; set; } = String.Empty;
  public string UrlKey { get; set; } = String.Empty;

  public string AlignmentCode { get; set; } = "alignment.lua";
  public bool Obfuscate { get; set; } = true;
  public int? DeviceSerial { get; set; } = null;
}

public class Device
{
  public int Serial { get; set; } = 0;
  public string MsgSubjectPublish { get; set; } = string.Empty;
  public string Org { get; set; } = string.Empty;
  public DeviceState State { get; set; } = DeviceState.Disconnect;

  public DateTime LastSeen { get; set; } = DateTime.MinValue;
}

public record DeviceError(Device Dev,string What);

public class Conveyor
{
  public int Id { get; set; } = 0;
  public string Name { get; set; } = string.Empty;
  public string Org { get; set; } = string.Empty;
  public string Site { get; set; } = string.Empty;
  public string LuaCode { get; set; } = string.Empty;

}