using Microsoft.EntityFrameworkCore;
using Microsoft.Extensions.Options;

namespace caas_gui.Data;

public class CaasDBContext : DbContext
{
  private readonly Action<DbContextOptionsBuilder> _dbSelect;

  public CaasDBContext(DbContextOptions<CaasDBContext> options, IOptions<AppSettings> config)
      : base(options)
  {
    _dbSelect = (DbContextOptionsBuilder optionsBuilder) =>
    {
      if (config.Value.DBBackend == DBBackend.Sqlite)
      {
        optionsBuilder.UseSqlite(config.Value.SqliteConnectionString);
      }
      else
      {
        optionsBuilder.UseNpgsql(config.Value.PostgresSqlConnectionString);
      }
    };
  }

  protected override void OnConfiguring(DbContextOptionsBuilder optionsBuilder)
  {
    _dbSelect(optionsBuilder);
  }
  protected override void OnModelCreating(ModelBuilder modelBuilder)
  {

    modelBuilder.Entity<Device>().HasKey(e => e.Serial);
    modelBuilder.Entity<Conveyor>().HasKey(e => e.Id);

  }

  public DbSet<Device> Devices { get; set; }
  public DbSet<Conveyor> Conveyors { get; set; }
}