using System;
using System.Collections.Generic;
using System.Threading.Tasks;
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

    }
}
