using System;
using Microsoft.EntityFrameworkCore.Migrations;

#nullable disable

namespace cadsgui.Migrations
{
    /// <inheritdoc />
    public partial class AddBeltfieldtoScans : Migration
    {
        /// <inheritdoc />
        protected override void Up(MigrationBuilder migrationBuilder)
        {
            migrationBuilder.DropColumn(
                name: "CordDiameter",
                table: "Conveyors");

            migrationBuilder.DropColumn(
                name: "Installed",
                table: "Conveyors");

            migrationBuilder.DropColumn(
                name: "PulleyCover",
                table: "Conveyors");

            migrationBuilder.DropColumn(
                name: "TopCover",
                table: "Conveyors");

            migrationBuilder.AddColumn<long>(
                name: "Belt",
                table: "Scans",
                type: "INTEGER",
                nullable: false,
                defaultValue: 0L);

            migrationBuilder.AddColumn<long>(
                name: "Belt",
                table: "Conveyors",
                type: "INTEGER",
                nullable: false,
                defaultValue: 0L);

            migrationBuilder.CreateTable(
                name: "Belts",
                columns: table => new
                {
                    Id = table.Column<long>(type: "INTEGER", nullable: false)
                        .Annotation("Sqlite:Autoincrement", true),
                    Conveyor = table.Column<long>(type: "INTEGER", nullable: false),
                    Installed = table.Column<DateTime>(type: "TEXT", nullable: false),
                    PulleyCover = table.Column<double>(type: "REAL", nullable: false),
                    CordDiameter = table.Column<double>(type: "REAL", nullable: false),
                    TopCover = table.Column<double>(type: "REAL", nullable: false),
                    Length = table.Column<double>(type: "REAL", nullable: false),
                    Width = table.Column<double>(type: "REAL", nullable: false),
                    Splices = table.Column<long>(type: "INTEGER", nullable: false)
                },
                constraints: table =>
                {
                    table.PrimaryKey("PK_Belts", x => x.Id);
                });
        }

        /// <inheritdoc />
        protected override void Down(MigrationBuilder migrationBuilder)
        {
            migrationBuilder.DropTable(
                name: "Belts");

            migrationBuilder.DropColumn(
                name: "Belt",
                table: "Scans");

            migrationBuilder.DropColumn(
                name: "Belt",
                table: "Conveyors");

            migrationBuilder.AddColumn<double>(
                name: "CordDiameter",
                table: "Conveyors",
                type: "REAL",
                nullable: false,
                defaultValue: 0.0);

            migrationBuilder.AddColumn<DateTime>(
                name: "Installed",
                table: "Conveyors",
                type: "TEXT",
                nullable: false,
                defaultValue: new DateTime(1, 1, 1, 0, 0, 0, 0, DateTimeKind.Unspecified));

            migrationBuilder.AddColumn<double>(
                name: "PulleyCover",
                table: "Conveyors",
                type: "REAL",
                nullable: false,
                defaultValue: 0.0);

            migrationBuilder.AddColumn<double>(
                name: "TopCover",
                table: "Conveyors",
                type: "REAL",
                nullable: false,
                defaultValue: 0.0);
        }
    }
}
