using Microsoft.EntityFrameworkCore.Migrations;
using Npgsql.EntityFrameworkCore.PostgreSQL.Metadata;

#nullable disable

namespace caasgui.Migrations
{
    /// <inheritdoc />
    public partial class AddConveyorsTable : Migration
    {
        /// <inheritdoc />
        protected override void Up(MigrationBuilder migrationBuilder)
        {
            migrationBuilder.RenameColumn(
                name: "MsgSubject",
                table: "Devices",
                newName: "Org");

            migrationBuilder.RenameColumn(
                name: "LuaCode",
                table: "Devices",
                newName: "MsgSubjectSubscribe");

            migrationBuilder.AddColumn<string>(
                name: "MsgSubjectPublish",
                table: "Devices",
                type: "text",
                nullable: false,
                defaultValue: "");

            migrationBuilder.CreateTable(
                name: "Conveyors",
                columns: table => new
                {
                    Id = table.Column<int>(type: "integer", nullable: false)
                        .Annotation("Npgsql:ValueGenerationStrategy", NpgsqlValueGenerationStrategy.IdentityByDefaultColumn),
                    Name = table.Column<string>(type: "text", nullable: false),
                    Org = table.Column<string>(type: "text", nullable: false),
                    LuaCode = table.Column<string>(type: "text", nullable: false)
                },
                constraints: table =>
                {
                    table.PrimaryKey("PK_Conveyors", x => x.Id);
                });
        }

        /// <inheritdoc />
        protected override void Down(MigrationBuilder migrationBuilder)
        {
            migrationBuilder.DropTable(
                name: "Conveyors");

            migrationBuilder.DropColumn(
                name: "MsgSubjectPublish",
                table: "Devices");

            migrationBuilder.RenameColumn(
                name: "Org",
                table: "Devices",
                newName: "MsgSubject");

            migrationBuilder.RenameColumn(
                name: "MsgSubjectSubscribe",
                table: "Devices",
                newName: "LuaCode");
        }
    }
}
