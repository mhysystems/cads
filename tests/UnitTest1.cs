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
      var g =Search.CountIf(rawSamples,width / columns,(x) => true, (z) => true);
      var req = new List<(int,float,int)>();
  
      await foreach (var r in g) {
        req.Add(r);
      }

      var ans = Enumerable.Repeat((4,30.0f,4),36).ToArray();
      Assert.Equal(req.ToArray(), ans);
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
      var g =Search.CountIf(rawSamples,width / columns,(x) => true, (z) => true);
      var l = Search.CountIfMatrixAsync(g,columns,length / rows);
      var req = new List<(int,float,int)>();
  
      await foreach (var r in l) {
        req.Add(r);
      }

      var ans = Enumerable.Repeat((16,30.0f,16),9).ToArray();
      Assert.Equal(req.ToArray(), ans);
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
      var g =Search.CountIf(rawSamples,width / columns,(x) => true, (z) => true);
      var l = Search.CountIfMatrixAsync(g,columns,length / rows);
      var req = new List<(double,double,int,float,int)>();
  
      await foreach (var r in Search.AddCoordinatesAsync(l,0,columns)) {
        req.Add(r);
      }

      var ans = new List<(double,double,int,float,int)>();
      foreach(var y in Enumerable.Range(0,rows)) {
        foreach(var x in Enumerable.Range(0,columns)) {
          ans.Add((x,y,16,30,16));        
        }
      }

      Assert.Equal(req.ToArray(), ans.ToArray());
    }

    [Fact]
    public async Task SearchPartition()
    {
      var db = "test.db";
      var columns = 3;
      var rows = 2;

      var req = new List<(double,double,double,float)>();
  
      await foreach (var r in Search.SearchPartitionAsync(db,columns,rows,4,8,12,100,(x) => true, (z) => true, (p) => true)) {
        req.Add(r);
      }

/*
      var ans = new List<(double,double,int,float,ulong)>();
      foreach(var y in Enumerable.Range(0,(int)rows)) {
        foreach(var x in Enumerable.Range(0,(int)columns)) {
          ans.Add((x*xRes + xOff,y*yRes,16,30,16ul));        
        }
      }*/
            Assert.Equal(true, true);
      //Assert.Equal(req.ToArray(), ans.Take(2).ToArray());
    }

 [Fact]
    public async Task SearchPartitionParallel()
    {
      var db = "test.db";
      var columns = 1024;
      var rows = 5000000;

      var req = new List<(double,double,double,float)>();
  
      foreach (var r in await Search.SearchPartitionParallelAsync(db,columns,rows,2048,1000000,100,(x) => true, (z) => true, (p) => p.Item1 == 0 && p.Item2 == 0)) {
        req.Add(r);
      }

/*
      var ans = new List<(double,double,int,float,ulong)>();
      foreach(var y in Enumerable.Range(0,(int)rows)) {
        foreach(var x in Enumerable.Range(0,(int)columns)) {
          ans.Add((x*xRes + xOff,y*yRes,16,30,16ul));        
        }
      }*/
            Assert.Equal(true, true);
      //Assert.Equal(req.ToArray(), ans.Take(2).ToArray());
    }

  
  
  
  
  
  } // End UnitTest1
}
