using System;
using Microsoft.EntityFrameworkCore.Migrations;

#nullable disable

namespace cadsgui.Migrations
{
    /// <inheritdoc />
    public partial class Init : Migration
    {
        /// <inheritdoc />
        protected override void Up(MigrationBuilder migrationBuilder)
        {
            migrationBuilder.CreateTable(
                name: "belt",
                columns: table => new
                {
                    rowid = table.Column<long>(type: "INTEGER", nullable: false)
                        .Annotation("Sqlite:Autoincrement", true),
                    site = table.Column<string>(type: "TEXT", nullable: false),
                    conveyor = table.Column<string>(type: "TEXT", nullable: false),
                    chrono = table.Column<DateTime>(type: "TEXT", nullable: false),
                    xres = table.Column<double>(name: "x_res", type: "REAL", nullable: false),
                    yres = table.Column<double>(name: "y_res", type: "REAL", nullable: false),
                    zres = table.Column<double>(name: "z_res", type: "REAL", nullable: false),
                    zoff = table.Column<double>(name: "z_off", type: "REAL", nullable: false),
                    zmax = table.Column<double>(name: "z_max", type: "REAL", nullable: false),
                    zmin = table.Column<double>(name: "z_min", type: "REAL", nullable: false),
                    Ymax = table.Column<double>(type: "REAL", nullable: false),
                    YmaxN = table.Column<double>(type: "REAL", nullable: false),
                    WidthN = table.Column<double>(type: "REAL", nullable: false)
                },
                constraints: table =>
                {
                    table.PrimaryKey("PK_belt", x => x.rowid);
                });

            migrationBuilder.CreateTable(
                name: "Conveyors",
                columns: table => new
                {
                    rowid = table.Column<long>(type: "INTEGER", nullable: false)
                        .Annotation("Sqlite:Autoincrement", true),
                    Site = table.Column<string>(type: "TEXT", nullable: false),
                    Belt = table.Column<string>(type: "TEXT", nullable: false),
                    Category = table.Column<string>(type: "TEXT", nullable: false),
                    Flow = table.Column<string>(type: "TEXT", nullable: false),
                    Cads = table.Column<int>(type: "INTEGER", nullable: false)
                },
                constraints: table =>
                {
                    table.PrimaryKey("PK_Conveyors", x => x.rowid);
                });

            migrationBuilder.CreateTable(
                name: "SavedZDepthParams",
                columns: table => new
                {
                    Name = table.Column<string>(type: "TEXT", nullable: false),
                    Site = table.Column<string>(type: "TEXT", nullable: false),
                    Conveyor = table.Column<string>(type: "TEXT", nullable: false),
                    Width = table.Column<double>(type: "REAL", nullable: false),
                    Length = table.Column<double>(type: "REAL", nullable: false),
                    Depth = table.Column<double>(type: "REAL", nullable: false),
                    Percentage = table.Column<double>(type: "REAL", nullable: false)
                },
                constraints: table =>
                {
                });
        }

        /// <inheritdoc />
        protected override void Down(MigrationBuilder migrationBuilder)
        {
            migrationBuilder.DropTable(
                name: "belt");

            migrationBuilder.DropTable(
                name: "Conveyors");

            migrationBuilder.DropTable(
                name: "SavedZDepthParams");
        }
    }
}
