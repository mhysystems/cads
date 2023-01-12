using Microsoft.EntityFrameworkCore;
using MudBlazor.Services;
using Fluxor;
using System.Globalization;

using cads_gui.Data;
using cads_gui.Services;

var builder = WebApplication.CreateBuilder(args);
var pathString = builder.Configuration.GetSection("webgui").GetValue<string>("DBPath") ?? String.Empty;
var connectionString = builder.Configuration.GetSection("webgui").GetValue<string>("ConnectionString") ?? String.Empty;

// Add services to the container.
builder.Services.AddRazorPages();
builder.Services.AddServerSideBlazor();
builder.Services.AddSignalR();

builder.Services.AddDbContextFactory<SQLiteDBContext>(options =>
  options.UseSqlite(connectionString)
);

builder.Services.AddFluxor(o => o.ScanAssemblies(typeof(Program).Assembly));
builder.Services.AddScoped<BeltService>();
builder.Services.AddMudServices();
builder.Services.AddHostedService<NatsConsumerHostedService>();
builder.Services.AddOptions();

builder.Services.Configure<AppSettings>(builder.Configuration.GetSection("webgui"));

var app = builder.Build();

// Configure the HTTP request pipeline.
if (!app.Environment.IsDevelopment())
{
    app.UseExceptionHandler("/Error");
    // The default HSTS value is 30 days. You may want to change this for production scenarios, see https://aka.ms/aspnetcore-hsts.
    app.UseHsts();
}

var cultureInfo = new CultureInfo("en-AU");
CultureInfo.DefaultThreadCurrentCulture = cultureInfo;
CultureInfo.DefaultThreadCurrentUICulture = cultureInfo;

app.UseHttpsRedirection();

app.UseStaticFiles();

app.UseRouting();

app.MapBlazorHub();
app.MapHub<RealtimeHub>("/realtime");
app.MapControllers();
app.MapFallbackToPage("/_Host");

app.Run();

