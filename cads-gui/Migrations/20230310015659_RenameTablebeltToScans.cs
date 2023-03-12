using Microsoft.EntityFrameworkCore.Migrations;

#nullable disable

namespace cadsgui.Migrations
{
    /// <inheritdoc />
    public partial class RenameTablebeltToScans : Migration
    {
        /// <inheritdoc />
        protected override void Up(MigrationBuilder migrationBuilder)
        {
            migrationBuilder.DropPrimaryKey(
                name: "PK_belt",
                table: "belt");

            migrationBuilder.RenameTable(
                name: "belt",
                newName: "Scans");

            migrationBuilder.AddPrimaryKey(
                name: "PK_Scans",
                table: "Scans",
                column: "rowid");
        }

        /// <inheritdoc />
        protected override void Down(MigrationBuilder migrationBuilder)
        {
            migrationBuilder.DropPrimaryKey(
                name: "PK_Scans",
                table: "Scans");

            migrationBuilder.RenameTable(
                name: "Scans",
                newName: "belt");

            migrationBuilder.AddPrimaryKey(
                name: "PK_belt",
                table: "belt",
                column: "rowid");
        }
    }
}
