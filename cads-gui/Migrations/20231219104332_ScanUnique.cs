using Microsoft.EntityFrameworkCore.Migrations;

#nullable disable

namespace cadsgui.Migrations
{
    /// <inheritdoc />
    public partial class ScanUnique : Migration
    {
        /// <inheritdoc />
        protected override void Up(MigrationBuilder migrationBuilder)
        {
            migrationBuilder.CreateIndex(
                name: "IX_Scans_ConveyorId_BeltId_Chrono",
                table: "Scans",
                columns: new[] { "ConveyorId", "BeltId", "Chrono" },
                unique: true);
        }

        /// <inheritdoc />
        protected override void Down(MigrationBuilder migrationBuilder)
        {
            migrationBuilder.DropIndex(
                name: "IX_Scans_ConveyorId_BeltId_Chrono",
                table: "Scans");
        }
    }
}
