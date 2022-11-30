using System;
using System.Collections.Generic;
using System.Threading.Tasks;
using System.Linq;
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
				  var t = await NoAsp.RetrieveFrameModular("whaleback-cv405-2022-08-07-084055", 0, 512,512);
          Assert.Equal(true,true);
        }

        [Fact]
        public async Task Test2()
        {
					var t = await NoAsp.RetrieveFrameAsync("whaleback-cv405-2022-08-07-084055", 900000, -10);
          Assert.Equal(true,true);
        }

        [Fact]
        public async Task RetrieveFrameForwardAsync()
        {
          var x = Directory.GetCurrentDirectory(); 
          var t0 = NoEFCore.RetrieveConveyorScanAsync("whaleback","cv405","/home/me/dev/cads/cads-gui/conveyors.db");
          var t2 = NoAsp.ConveyorsHeightAsync(t0,10.0, 40);

          //var ff = NoAsp.RetrieveFrameForwardAsync("whaleback-cv405-2022-08-07-084055", 921970, 2);
          //await ff.
					await foreach (var p in t2) {
            var dbg = p;
          }
          Assert.Equal(true,true);
        }

    }
}
