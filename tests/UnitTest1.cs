using System;
using System.Collections.Generic;
using System.Threading.Tasks;
using System.Linq;
using System.Diagnostics;

using Microsoft.Extensions.Configuration;
using Xunit;
using cads_gui.Data;
using cads_gui.BeltN;

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
    public async Task RetrieveFrameSamplesAsync()
    {
      var ans = new float[]{1,2,3,4};
      var req = new List<float>();
      await foreach (var r in Search.RetrieveFrameSamplesAsync(0,3,"test.db")) {
        req.AddRange(r.Item2);
      }
      Assert.Equal(ans, req.ToArray());
    }

  }
}
