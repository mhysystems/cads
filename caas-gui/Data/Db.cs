using Microsoft.EntityFrameworkCore;

namespace caas_gui.Data;

public class Db
{
  public static Device[] GetDevices(IDbContextFactory<PostgresDBContext> dBContext) {

    using var context = dBContext.CreateDbContext();
    var rows = context?.Devices?.AsEnumerable();

    if(rows is null || !rows.Any()) return Array.Empty<Device>();

    return rows.ToArray();
  }

  public static Device? GetDevice(IDbContextFactory<PostgresDBContext> dBContext, int serial) {

    using var context = dBContext.CreateDbContext();
    var rows = context?.Devices?.Where( r => r.Serial == serial);

    if(rows is null || !rows.Any()) return null;

    return rows.First();
  }


}