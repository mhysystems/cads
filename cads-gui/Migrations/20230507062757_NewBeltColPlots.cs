using Microsoft.EntityFrameworkCore.Migrations;

#nullable disable

namespace cadsgui.Migrations
{
    /// <inheritdoc />
    public partial class NewBeltColPlots : Migration
    {
        /// <inheritdoc />
        protected override void Up(MigrationBuilder migrationBuilder)
        {
            migrationBuilder.AddColumn<long>(
                name: "Belt",
                table: "Plots",
                type: "INTEGER",
                nullable: false,
                defaultValue: 0L);
        }

        /// <inheritdoc />
        protected override void Down(MigrationBuilder migrationBuilder)
        {
            migrationBuilder.DropColumn(
                name: "Belt",
                table: "Plots");
        }
    }
}
