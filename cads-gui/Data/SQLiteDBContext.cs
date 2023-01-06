using Microsoft.EntityFrameworkCore;
using System;

namespace cads_gui.Data
{
    public class SQLiteDBContext : DbContext
    {   
        public SQLiteDBContext(DbContextOptions<SQLiteDBContext> options)
            : base(options)
        {
          Database.EnsureCreated();
        }

        protected override void OnModelCreating(ModelBuilder modelBuilder) {

					modelBuilder.Entity<Belt>().ToTable("BELTINFO").HasKey(e => e.rowid);
          modelBuilder.Entity<Belt>().ToTable("BELTINFO").Property(e => e.chrono).HasConversion(e => e, e => DateTime.SpecifyKind(e,DateTimeKind.Utc));
					modelBuilder.Entity<SavedZDepthParams>().ToTable("SAVEDZDEPTHPARAMS").HasKey(e => e.rowid);
					modelBuilder.Entity<Conveyors>().ToTable("conveyors").HasKey(e => e.rowid);
        }


       	public DbSet<Belt> belt { get; set; }
				public DbSet<SavedZDepthParams> SavedZDepthParams { get; set; }
				public DbSet<Conveyors> Conveyors { get; set; }
    }
}