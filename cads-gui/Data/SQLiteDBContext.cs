using Microsoft.EntityFrameworkCore;
using TimeZoneConverter;

namespace cads_gui.Data
{
    public class SQLiteDBContext : DbContext
    {   
        public SQLiteDBContext(DbContextOptions<SQLiteDBContext> options)
            : base(options)
        {
          //Database.EnsureCreated();
          //Database.Migrate();
        }
					string c(string timezone) {
            if (!TimeZoneInfo.TryConvertWindowsIdToIanaId(timezone, out string? iana)) {
                //throw new TimeZoneNotFoundException($"No IANA time zone found for {timezone}.");
            }
              return iana ?? "Etc/UTC";
          }
        protected override void OnModelCreating(ModelBuilder modelBuilder) {
          
          modelBuilder.Entity<Scan>().HasKey(e => e.Id);
          modelBuilder.Entity<Scan>().Property(b => b.Id).ValueGeneratedOnAdd();
          modelBuilder.Entity<Scan>().Property(e => e.Chrono).HasConversion(e => e, e => DateTime.SpecifyKind(e,DateTimeKind.Utc));
          modelBuilder.Entity<Scan>().HasIndex(e => new {e.ConveyorId,e.BeltId,e.Chrono}).IsUnique();
					modelBuilder.Entity<SavedZDepthParams>().HasKey( e => new{e.Site,e.Conveyor,e.Name});
					modelBuilder.Entity<Conveyor>().HasKey(e => e.Id);
          modelBuilder.Entity<Conveyor>().Property(b => b.Id).ValueGeneratedOnAdd();
          modelBuilder.Entity<Conveyor>().Property(e => e.Timezone).HasConversion(
            e => e.Id,
            e => TimeZoneInfo.FindSystemTimeZoneById(e)
            );
          
          modelBuilder.Entity<Belt>().HasKey(e => e.Id);
          modelBuilder.Entity<Belt>().Property(b => b.Id).ValueGeneratedOnAdd();
          modelBuilder.Entity<Belt>().HasIndex(e => e.Serial).IsUnique();

          modelBuilder.Entity<BeltInstall>().HasKey(e => e.Id);
          modelBuilder.Entity<BeltInstall>().Property(b => b.Id).ValueGeneratedOnAdd();

          modelBuilder.Entity<Grafana>().HasKey(e => e.Id);
          modelBuilder.Entity<Grafana>().Property(b => b.Id).ValueGeneratedOnAdd();
        }


       	public DbSet<Scan> Scans { get; set; }
        public DbSet<Belt> Belts { get; set; }
        public DbSet<Conveyor> Conveyors { get; set; }
        public DbSet<BeltInstall> BeltInstalls { get; set; }
				public DbSet<SavedZDepthParams> SavedZDepthParams { get; set; }
        public DbSet<Grafana> Plots { get; set;}
    }
}