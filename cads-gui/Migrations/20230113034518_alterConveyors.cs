using Microsoft.EntityFrameworkCore.Migrations;

#nullable disable

namespace cadsgui.Migrations
{
    /// <inheritdoc />
    public partial class alterConveyors : Migration
    {
        /// <inheritdoc />
        protected override void Up(MigrationBuilder migrationBuilder)
        {
            migrationBuilder.DropPrimaryKey(
                name: "PK_Conveyors",
                table: "Conveyors");

            migrationBuilder.DropColumn(
                name: "rowid",
                table: "Conveyors");

            migrationBuilder.DropColumn(
                name: "Belt",
                table: "Conveyors");

            migrationBuilder.RenameColumn(
                name: "Flow",
                table: "Conveyors",
                newName: "Timezone");

            migrationBuilder.RenameColumn(
                name: "Category",
                table: "Conveyors",
                newName: "Name");

            migrationBuilder.RenameColumn(
                name: "Cads",
                table: "Conveyors",
                newName: "Id");

            migrationBuilder.AlterColumn<int>(
                name: "Id",
                table: "Conveyors",
                type: "INTEGER",
                nullable: false,
                oldClrType: typeof(int),
                oldType: "INTEGER")
                .Annotation("Sqlite:Autoincrement", true);

            migrationBuilder.AddColumn<double>(
                name: "CordDiameter",
                table: "Conveyors",
                type: "REAL",
                nullable: false,
                defaultValue: 0.0);

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

            migrationBuilder.AddPrimaryKey(
                name: "PK_Conveyors",
                table: "Conveyors",
                column: "Id");
        }

        /// <inheritdoc />
        protected override void Down(MigrationBuilder migrationBuilder)
        {
            migrationBuilder.DropPrimaryKey(
                name: "PK_Conveyors",
                table: "Conveyors");

            migrationBuilder.DropColumn(
                name: "CordDiameter",
                table: "Conveyors");

            migrationBuilder.DropColumn(
                name: "PulleyCover",
                table: "Conveyors");

            migrationBuilder.DropColumn(
                name: "TopCover",
                table: "Conveyors");

            migrationBuilder.RenameColumn(
                name: "Timezone",
                table: "Conveyors",
                newName: "Flow");

            migrationBuilder.RenameColumn(
                name: "Name",
                table: "Conveyors",
                newName: "Category");

            migrationBuilder.RenameColumn(
                name: "Id",
                table: "Conveyors",
                newName: "Cads");

            migrationBuilder.AlterColumn<int>(
                name: "Cads",
                table: "Conveyors",
                type: "INTEGER",
                nullable: false,
                oldClrType: typeof(int),
                oldType: "INTEGER")
                .OldAnnotation("Sqlite:Autoincrement", true);

            migrationBuilder.AddColumn<long>(
                name: "rowid",
                table: "Conveyors",
                type: "INTEGER",
                nullable: false,
                defaultValue: 0L)
                .Annotation("Sqlite:Autoincrement", true);

            migrationBuilder.AddColumn<string>(
                name: "Belt",
                table: "Conveyors",
                type: "TEXT",
                nullable: false,
                defaultValue: "");

            migrationBuilder.AddPrimaryKey(
                name: "PK_Conveyors",
                table: "Conveyors",
                column: "rowid");
        }
    }
}
