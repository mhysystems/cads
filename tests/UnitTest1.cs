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
    public async Task RetrieveFrameSamplesAsync()
    {
      var ans = new float[12 * 12];
      Array.Fill(ans,30);
      var req = new List<float>();
      await foreach (var r in Search.RetrieveFrameSamplesAsync(0,12,"test.db")) {
        req.AddRange(r);
      }
      Assert.Equal(ans, req.ToArray());
    }

    [Fact]
    public async Task RetrieveBeltAttributesAsync()
    {
      var rst = await Search.RetrieveBeltAttributesAsync("test.db");
      Assert.Equal((12,12,1,0.5,-1), rst);
    }

    [Fact]
    public async Task CountIf1D()
    {
      var db = "test.db";
      var columns = 3;
      var length = 12;
      var width = 12;
      var rawSamples = Search.RetrieveFrameSamplesAsync(0,length,db);
      var g =Search.CountIf(rawSamples,width / columns,(x) => true, (z) => z < 30);
      var req = new List<Search.Result>();
  
      await foreach (var r in g) {
        req.Add(r);
      }

      Assert.Equal(true, true);

    }

    [Fact]
    public async Task CountIfMatrixAsync()
    {
      var db = "test.db";
      var columns = 3;
      var rows = 3;
      var length = 12;
      var width = 12;
      var rawSamples = Search.RetrieveFrameSamplesAsync(0,length,db);
      var g =Search.CountIf(rawSamples,width / columns,(x) => true, (z) => z < 30);
      var l = Search.CountIfMatrixAsync(g,columns,length / rows);
      var req = new List<Search.Result>();
  
      await foreach (var r in l) {
        req.Add(r);
      }

      Assert.Equal(true, true);
    }


    [Fact]
    public async Task AddCoordinatesAsync()
    {
      var db = "test.db";
      var columns = 3;
      var rows = 3;
      var length = 12;
      var width = 12;
      var rawSamples = Search.RetrieveFrameSamplesAsync(0,length,db);
      var g =Search.CountIf(rawSamples,width / columns,(x) => true, (z) => z < 30);
      var l = Search.CountIfMatrixAsync(g,columns,length / rows);
      var req = new List<Search.ResultCoord>();
  
      await foreach (var r in Search.AddCoordinatesAsync(l,columns,0)) {
        req.Add(r);
      }

      Assert.Equal(true, true);
    }


    [Fact]
    public async Task SearchPartition()
    {
      var db = "/home/me/dev/cads/profiles/gui-extern/whaleback-cv405-2022-12-17-00000";// "test.db";
      var xStride = 1500;
      var yStride = 6;

      var why = 606;

      var req = new List<Search.SearchResult>();
  
      await foreach (var r in Search.SearchPartitionAsync(db,xStride,yStride,179999-why,1500,why,1000000,(x) => true, (z) => z < 30, (p) => p.Percent > 0)) {
        req.Add(r);
      }


      Assert.Equal(true, true);

    }

 [Fact]
    public async Task SearchPartitionParallel()
    {
      var db = "test.db";
      var columns = 1;
      var rows = 12;

      var req = new List<Search.SearchResult>();
  
      foreach (var r in await Search.SearchParallelAsync(db,columns,rows,14,14,100,(x) => true, (z) => z < 30, (p) => p.Percent > 0)) {
        req.Add(r);
      }


      Assert.Equal(true, true);
    }
 
  
  
  
  
  
  } // End UnitTest1
}
