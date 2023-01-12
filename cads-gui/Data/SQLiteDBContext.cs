using Microsoft.EntityFrameworkCore;
using System;

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

        protected override void OnModelCreating(ModelBuilder modelBuilder) {

					modelBuilder.Entity<Belt>().HasKey(e => e.rowid);
          modelBuilder.Entity<Belt>().Property(b => b.rowid).ValueGeneratedOnAdd();
          modelBuilder.Entity<Belt>().Ignore(b => b.ConveyorID);
          modelBuilder.Entity<Belt>().Ignore(b => b.name);
          modelBuilder.Entity<Belt>().Property(e => e.chrono).HasConversion(e => e, e => DateTime.SpecifyKind(e,DateTimeKind.Utc));
					modelBuilder.Entity<SavedZDepthParams>().HasNoKey();
					modelBuilder.Entity<Conveyors>().HasKey(e => e.rowid);
          modelBuilder.Entity<Conveyors>().Property(b => b.rowid).ValueGeneratedOnAdd();
        }


       	public DbSet<Belt> belt { get; set; }
				public DbSet<SavedZDepthParams> SavedZDepthParams { get; set; }
				public DbSet<Conveyors> Conveyors { get; set; }
    }
}