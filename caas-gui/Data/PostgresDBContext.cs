using Microsoft.EntityFrameworkCore;
using Microsoft.Extensions.Options;

namespace caas_gui.Data;

public class PostgresDBContext : DbContext
{
    private readonly string _connectionString = string.Empty;   
    public PostgresDBContext(DbContextOptions<PostgresDBContext> options, IOptions<AppSettings> config)
        : base(options)
    {
      _connectionString = config.Value.ConnectionString;
      //Database.EnsureCreated();
      //Database.Migrate();
    }

    protected override void OnConfiguring(DbContextOptionsBuilder optionsBuilder) {
      optionsBuilder.UseNpgsql(_connectionString);
    }
    protected override void OnModelCreating(ModelBuilder modelBuilder) {

      modelBuilder.Entity<Device>().HasKey(e => e.Serial);

    }

    public DbSet<Device> Devices { get; set; }
}