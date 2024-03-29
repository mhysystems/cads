﻿// <auto-generated />
using System;
using Microsoft.EntityFrameworkCore;
using Microsoft.EntityFrameworkCore.Infrastructure;
using Microsoft.EntityFrameworkCore.Storage.ValueConversion;
using caas_gui.Data;

#nullable disable

namespace caasgui.Migrations
{
    [DbContext(typeof(CaasDBContext))]
    partial class PostgresDBContextModelSnapshot : ModelSnapshot
    {
        protected override void BuildModel(ModelBuilder modelBuilder)
        {
#pragma warning disable 612, 618
            modelBuilder.HasAnnotation("ProductVersion", "7.0.5");

            modelBuilder.Entity("caas_gui.Data.Conveyor", b =>
                {
                    b.Property<int>("Id")
                        .ValueGeneratedOnAdd()
                        .HasColumnType("INTEGER");

                    b.Property<string>("LuaCode")
                        .IsRequired()
                        .HasColumnType("TEXT");

                    b.Property<string>("Name")
                        .IsRequired()
                        .HasColumnType("TEXT");

                    b.Property<string>("Org")
                        .IsRequired()
                        .HasColumnType("TEXT");

                    b.Property<string>("Site")
                        .IsRequired()
                        .HasColumnType("TEXT");

                    b.HasKey("Id");

                    b.ToTable("Conveyors");
                });

            modelBuilder.Entity("caas_gui.Data.Device", b =>
                {
                    b.Property<int>("Serial")
                        .ValueGeneratedOnAdd()
                        .HasColumnType("INTEGER");

                    b.Property<DateTime>("LastSeen")
                        .HasColumnType("TEXT");

                    b.Property<string>("MsgSubjectPublish")
                        .IsRequired()
                        .HasColumnType("TEXT");

                    b.Property<string>("Org")
                        .IsRequired()
                        .HasColumnType("TEXT");

                    b.Property<int>("State")
                        .HasColumnType("INTEGER");

                    b.HasKey("Serial");

                    b.ToTable("Devices");
                });
#pragma warning restore 612, 618
        }
    }
}
