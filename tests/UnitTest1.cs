using System;
using System.Collections.Generic;
using System.Threading.Tasks;
using System.Linq;
using System.Diagnostics;

using Microsoft.Extensions.Configuration;
using Xunit;
using cads_gui.Data;

using System.IO;

namespace tests
{
  public class UnitTest1
  {
    [Fact]
    public async Task Test1()
    {
      var t = await NoAsp.RetrieveFrameModular("whaleback-cv405-2022-08-07-084055", 0, 512, 512);
      Assert.Equal(true, true);
    }

    [Fact]
    public async Task Test2()
    {
      var t = await NoAsp.RetrieveFrameAsync("whaleback-cv405-2022-08-07-084055", 900000, -10);
      Assert.Equal(true, true);
    }

    [Fact]
    public async Task RetrieveFrameForwardAsync()
    {
      var configuration = new ConfigurationBuilder()
            .SetBasePath(Directory.GetCurrentDirectory())
            .AddJsonFile("appsettings.json")
            .Build();
      var pathString = configuration.GetSection("webgui").GetValue<string>("DBPath") ?? String.Empty;
      var dbpath = Path.GetFullPath(Path.Combine(pathString, "conveyors.db"));
      var t0 = NoEFCore.RetrieveConveyorScanAsync("whaleback", "cv405", dbpath);
      var belts = t0.Select(x => (x.Item1, Path.GetFullPath(Path.Combine(pathString, x.Item2))));
      List<DateTime> x = new();
      List<double> y = new();

      var timer = new Stopwatch();
      timer.Start();
      foreach (var (chrono, height) in await NoAsp.ConveyorsHeightAsync(belts,0, 40))
      {
        x.Add(chrono);
        y.Add(height);
      };

      timer.Stop();
      Assert.Equal(true, true);
    }

  }
}
