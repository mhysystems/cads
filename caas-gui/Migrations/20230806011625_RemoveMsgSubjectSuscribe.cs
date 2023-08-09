using Microsoft.EntityFrameworkCore.Migrations;

#nullable disable

namespace caasgui.Migrations
{
    /// <inheritdoc />
    public partial class RemoveMsgSubjectSuscribe : Migration
    {
        /// <inheritdoc />
        protected override void Up(MigrationBuilder migrationBuilder)
        {
            migrationBuilder.DropColumn(
                name: "MsgSubjectSubscribe",
                table: "Devices");
        }

        /// <inheritdoc />
        protected override void Down(MigrationBuilder migrationBuilder)
        {
            migrationBuilder.AddColumn<string>(
                name: "MsgSubjectSubscribe",
                table: "Devices",
                type: "text",
                nullable: false,
                defaultValue: "");
        }
    }
}
