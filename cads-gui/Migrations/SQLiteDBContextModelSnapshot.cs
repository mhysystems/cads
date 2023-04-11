﻿// <auto-generated />
using System;
using Microsoft.EntityFrameworkCore;
using Microsoft.EntityFrameworkCore.Infrastructure;
using Microsoft.EntityFrameworkCore.Storage.ValueConversion;
using cads_gui.Data;

#nullable disable

namespace cadsgui.Migrations
{
    [DbContext(typeof(SQLiteDBContext))]
    partial class SQLiteDBContextModelSnapshot : ModelSnapshot
    {
        protected override void BuildModel(ModelBuilder modelBuilder)
        {
#pragma warning disable 612, 618
            modelBuilder.HasAnnotation("ProductVersion", "7.0.2");

            modelBuilder.Entity("cads_gui.Data.Belt", b =>
                {
                    b.Property<long>("Id")
                        .ValueGeneratedOnAdd()
                        .HasColumnType("INTEGER");

                    b.Property<long>("Conveyor")
                        .HasColumnType("INTEGER");

                    b.Property<double>("CordDiameter")
                        .HasColumnType("REAL");

                    b.Property<DateTime>("Installed")
                        .HasColumnType("TEXT");

                    b.Property<double>("Length")
                        .HasColumnType("REAL");

                    b.Property<double>("PulleyCover")
                        .HasColumnType("REAL");

                    b.Property<long>("Splices")
                        .HasColumnType("INTEGER");

                    b.Property<double>("TopCover")
                        .HasColumnType("REAL");

                    b.Property<double>("Width")
                        .HasColumnType("REAL");

                    b.HasKey("Id");

                    b.ToTable("Belts");
                });

            modelBuilder.Entity("cads_gui.Data.Conveyor", b =>
                {
                    b.Property<long>("Id")
                        .ValueGeneratedOnAdd()
                        .HasColumnType("INTEGER");

                    b.Property<long>("Belt")
                        .HasColumnType("INTEGER");

                    b.Property<string>("Name")
                        .IsRequired()
                        .HasColumnType("TEXT");

                    b.Property<double>("PulleyCircumference")
                        .HasColumnType("REAL");

                    b.Property<string>("Site")
                        .IsRequired()
                        .HasColumnType("TEXT");

                    b.Property<string>("Timezone")
                        .IsRequired()
                        .HasColumnType("TEXT");

                    b.HasKey("Id");

                    b.ToTable("Conveyors");
                });

            modelBuilder.Entity("cads_gui.Data.SavedZDepthParams", b =>
                {
                    b.Property<string>("Site")
                        .HasColumnType("TEXT");

                    b.Property<string>("Conveyor")
                        .HasColumnType("TEXT");

                    b.Property<string>("Name")
                        .HasColumnType("TEXT");

                    b.Property<double>("Length")
                        .HasColumnType("REAL");

                    b.Property<double>("Percentage")
                        .HasColumnType("REAL");

                    b.Property<double>("Width")
                        .HasColumnType("REAL");

                    b.Property<double>("XMax")
                        .HasColumnType("REAL");

                    b.Property<double>("XMin")
                        .HasColumnType("REAL");

                    b.Property<double>("ZMax")
                        .HasColumnType("REAL");

                    b.HasKey("Site", "Conveyor", "Name");

                    b.ToTable("SavedZDepthParams");
                });

            modelBuilder.Entity("cads_gui.Data.Scan", b =>
                {
                    b.Property<long>("rowid")
                        .ValueGeneratedOnAdd()
                        .HasColumnType("INTEGER");

                    b.Property<long>("Belt")
                        .HasColumnType("INTEGER");

                    b.Property<int>("Orientation")
                        .HasColumnType("INTEGER");

                    b.Property<double>("WidthN")
                        .HasColumnType("REAL");

                    b.Property<double>("Ymax")
                        .HasColumnType("REAL");

                    b.Property<double>("YmaxN")
                        .HasColumnType("REAL");

                    b.Property<DateTime>("chrono")
                        .HasColumnType("TEXT");

                    b.Property<string>("conveyor")
                        .IsRequired()
                        .HasColumnType("TEXT");

                    b.Property<string>("site")
                        .IsRequired()
                        .HasColumnType("TEXT");

                    b.Property<double>("x_res")
                        .HasColumnType("REAL");

                    b.Property<double>("y_res")
                        .HasColumnType("REAL");

                    b.Property<double>("z_max")
                        .HasColumnType("REAL");

                    b.Property<double>("z_min")
                        .HasColumnType("REAL");

                    b.Property<double>("z_off")
                        .HasColumnType("REAL");

                    b.Property<double>("z_res")
                        .HasColumnType("REAL");

                    b.HasKey("rowid");

                    b.ToTable("Scans");
                });
#pragma warning restore 612, 618
        }
    }
}
