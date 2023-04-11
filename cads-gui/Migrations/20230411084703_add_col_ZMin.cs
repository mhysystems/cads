using Microsoft.EntityFrameworkCore.Migrations;

#nullable disable

namespace cadsgui.Migrations
{
    /// <inheritdoc />
    public partial class addcolZMin : Migration
    {
        /// <inheritdoc />
        protected override void Up(MigrationBuilder migrationBuilder)
        {
            migrationBuilder.AddColumn<double>(
                name: "ZMin",
                table: "SavedZDepthParams",
                type: "REAL",
                nullable: false,
                defaultValue: 0.0);
        }

        /// <inheritdoc />
        protected override void Down(MigrationBuilder migrationBuilder)
        {
            migrationBuilder.DropColumn(
                name: "ZMin",
                table: "SavedZDepthParams");
        }
    }
}
