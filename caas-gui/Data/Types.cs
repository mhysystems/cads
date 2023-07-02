
namespace caas_gui.Data;

public class AppSettings
{
  public string NatsUrl { get; set; } = "127.0.0.1";
  public string ConnectionString { get; set; } = String.Empty;
  public string UrlKey { get; set; } = String.Empty;

}

public class Device
{
  public int Serial { get; set; } = 0;
  public string MsgSubjectPublish { get; set; } = string.Empty;
  public string MsgSubjectSubscribe { get; set; } = string.Empty;
  public string Org { get; set; } = string.Empty;
  public long State { get; set; } = 0;
}

public class Conveyor
{
  public int Id { get; set; } = 0;
  public string Name { get; set; } = string.Empty;
  public string Org { get; set; } = string.Empty;
  public string LuaCode { get; set; } = string.Empty;

}