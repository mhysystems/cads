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
            if (!TimeZoneInfo.TryConvertWindowsIdToIanaId(timezone, out string iana)) {
                //throw new TimeZoneNotFoundException($"No IANA time zone found for {timezone}.");
            }
              return iana;
          }
        protected override void OnModelCreating(ModelBuilder modelBuilder) {


          
          modelBuilder.Entity<Belt>().HasKey(e => e.rowid);
          modelBuilder.Entity<Belt>().Property(b => b.rowid).ValueGeneratedOnAdd();
          modelBuilder.Entity<Belt>().Ignore(b => b.ConveyorID);
          modelBuilder.Entity<Belt>().Ignore(b => b.name);
          modelBuilder.Entity<Belt>().Property(e => e.chrono).HasConversion(e => e, e => DateTime.SpecifyKind(e,DateTimeKind.Utc));
					modelBuilder.Entity<SavedZDepthParams>().HasNoKey();
					modelBuilder.Entity<Conveyor>().HasKey(e => e.Id);
          modelBuilder.Entity<Conveyor>().Property(b => b.Id).ValueGeneratedOnAdd();
          modelBuilder.Entity<Conveyor>().Property(e => e.Timezone).HasConversion(
            e => e.Id,
            e => TimeZoneInfo.FindSystemTimeZoneById(e)
            );
        }


       	public DbSet<Belt> belt { get; set; }
				public DbSet<SavedZDepthParams> SavedZDepthParams { get; set; }
				public DbSet<Conveyor> Conveyors { get; set; }
    }
}