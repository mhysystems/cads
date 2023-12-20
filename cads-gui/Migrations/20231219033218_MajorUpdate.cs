using System;
using Microsoft.EntityFrameworkCore.Migrations;

#nullable disable

namespace cadsgui.Migrations
{
    /// <inheritdoc />
    public partial class MajorUpdate : Migration
    {
        /// <inheritdoc />
        protected override void Up(MigrationBuilder migrationBuilder)
        {
            migrationBuilder.DropPrimaryKey(
                name: "PK_Scans",
                table: "Scans");

            migrationBuilder.DropColumn(
                name: "WidthN",
                table: "Scans");

            migrationBuilder.DropColumn(
                name: "Ymax",
                table: "Scans");

            migrationBuilder.DropColumn(
                name: "YmaxN",
                table: "Scans");

            migrationBuilder.DropColumn(
                name: "conveyor",
                table: "Scans");

            migrationBuilder.DropColumn(
                name: "x_res",
                table: "Scans");

            migrationBuilder.DropColumn(
                name: "y_res",
                table: "Scans");

            migrationBuilder.DropColumn(
                name: "z_max",
                table: "Scans");

            migrationBuilder.DropColumn(
                name: "z_min",
                table: "Scans");

            migrationBuilder.DropColumn(
                name: "z_off",
                table: "Scans");

            migrationBuilder.DropColumn(
                name: "z_res",
                table: "Scans");

            migrationBuilder.DropColumn(
                name: "Belt",
                table: "Conveyors");

            migrationBuilder.DropColumn(
                name: "Conveyor",
                table: "Belts");

            migrationBuilder.DropColumn(
                name: "Splices",
                table: "Belts");

            migrationBuilder.RenameColumn(
                name: "chrono",
                table: "Scans",
                newName: "Chrono");

            migrationBuilder.RenameColumn(
                name: "site",
                table: "Scans",
                newName: "Filepath");

            migrationBuilder.RenameColumn(
                name: "Orientation",
                table: "Scans",
                newName: "Status");

            migrationBuilder.RenameColumn(
                name: "Belt",
                table: "Scans",
                newName: "ConveyorId");

            migrationBuilder.RenameColumn(
                name: "rowid",
                table: "Scans",
                newName: "BeltId");

            migrationBuilder.RenameColumn(
                name: "Installed",
                table: "Belts",
                newName: "Serial");

            migrationBuilder.AlterColumn<long>(
                name: "BeltId",
                table: "Scans",
                type: "INTEGER",
                nullable: false,
                oldClrType: typeof(long),
                oldType: "INTEGER")
                .OldAnnotation("Sqlite:Autoincrement", true);

            migrationBuilder.AddColumn<long>(
                name: "Id",
                table: "Scans",
                type: "INTEGER",
                nullable: false,
                defaultValue: 0L)
                .Annotation("Sqlite:Autoincrement", true);

            migrationBuilder.AddColumn<double>(
                name: "TypicalSpeed",
                table: "Conveyors",
                type: "REAL",
                nullable: false,
                defaultValue: 0.0);

            migrationBuilder.AddPrimaryKey(
                name: "PK_Scans",
                table: "Scans",
                column: "Id");

            migrationBuilder.CreateTable(
                name: "BeltInstalls",
                columns: table => new
                {
                    Id = table.Column<long>(type: "INTEGER", nullable: false)
                        .Annotation("Sqlite:Autoincrement", true),
                    ConveyorId = table.Column<long>(type: "INTEGER", nullable: false),
                    BeltId = table.Column<long>(type: "INTEGER", nullable: false),
                    Chrono = table.Column<DateTime>(type: "TEXT", nullable: false)
                },
                constraints: table =>
                {
                    table.PrimaryKey("PK_BeltInstalls", x => x.Id);
                });

            migrationBuilder.CreateIndex(
                name: "IX_Belts_Serial",
                table: "Belts",
                column: "Serial",
                unique: true);
        }

        /// <inheritdoc />
        protected override void Down(MigrationBuilder migrationBuilder)
        {
            migrationBuilder.DropTable(
                name: "BeltInstalls");

            migrationBuilder.DropPrimaryKey(
                name: "PK_Scans",
                table: "Scans");

            migrationBuilder.DropIndex(
                name: "IX_Belts_Serial",
                table: "Belts");

            migrationBuilder.DropColumn(
                name: "Id",
                table: "Scans");

            migrationBuilder.DropColumn(
                name: "TypicalSpeed",
                table: "Conveyors");

            migrationBuilder.RenameColumn(
                name: "Chrono",
                table: "Scans",
                newName: "chrono");

            migrationBuilder.RenameColumn(
                name: "Status",
                table: "Scans",
                newName: "Orientation");

            migrationBuilder.RenameColumn(
                name: "Filepath",
                table: "Scans",
                newName: "site");

            migrationBuilder.RenameColumn(
                name: "ConveyorId",
                table: "Scans",
                newName: "Belt");

            migrationBuilder.RenameColumn(
                name: "BeltId",
                table: "Scans",
                newName: "rowid");

            migrationBuilder.RenameColumn(
                name: "Serial",
                table: "Belts",
                newName: "Installed");

            migrationBuilder.AlterColumn<long>(
                name: "rowid",
                table: "Scans",
                type: "INTEGER",
                nullable: false,
                oldClrType: typeof(long),
                oldType: "INTEGER")
                .Annotation("Sqlite:Autoincrement", true);

            migrationBuilder.AddColumn<double>(
                name: "WidthN",
                table: "Scans",
                type: "REAL",
                nullable: false,
                defaultValue: 0.0);

            migrationBuilder.AddColumn<double>(
                name: "Ymax",
                table: "Scans",
                type: "REAL",
                nullable: false,
                defaultValue: 0.0);

            migrationBuilder.AddColumn<double>(
                name: "YmaxN",
                table: "Scans",
                type: "REAL",
                nullable: false,
                defaultValue: 0.0);

            migrationBuilder.AddColumn<string>(
                name: "conveyor",
                table: "Scans",
                type: "TEXT",
                nullable: false,
                defaultValue: "");

            migrationBuilder.AddColumn<double>(
                name: "x_res",
                table: "Scans",
                type: "REAL",
                nullable: false,
                defaultValue: 0.0);

            migrationBuilder.AddColumn<double>(
                name: "y_res",
                table: "Scans",
                type: "REAL",
                nullable: false,
                defaultValue: 0.0);

            migrationBuilder.AddColumn<double>(
                name: "z_max",
                table: "Scans",
                type: "REAL",
                nullable: false,
                defaultValue: 0.0);

            migrationBuilder.AddColumn<double>(
                name: "z_min",
                table: "Scans",
                type: "REAL",
                nullable: false,
                defaultValue: 0.0);

            migrationBuilder.AddColumn<double>(
                name: "z_off",
                table: "Scans",
                type: "REAL",
                nullable: false,
                defaultValue: 0.0);

            migrationBuilder.AddColumn<double>(
                name: "z_res",
                table: "Scans",
                type: "REAL",
                nullable: false,
                defaultValue: 0.0);

            migrationBuilder.AddColumn<long>(
                name: "Belt",
                table: "Conveyors",
                type: "INTEGER",
                nullable: false,
                defaultValue: 0L);

            migrationBuilder.AddColumn<long>(
                name: "Conveyor",
                table: "Belts",
                type: "INTEGER",
                nullable: false,
                defaultValue: 0L);

            migrationBuilder.AddColumn<long>(
                name: "Splices",
                table: "Belts",
                type: "INTEGER",
                nullable: false,
                defaultValue: 0L);

            migrationBuilder.AddPrimaryKey(
                name: "PK_Scans",
                table: "Scans",
                column: "rowid");
        }
    }
}
