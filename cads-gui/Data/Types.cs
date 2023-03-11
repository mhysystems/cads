using System.Collections.Generic;
using System.Linq;
using System.ComponentModel.DataAnnotations.Schema;
using System;

namespace cads_gui.Data
{
  public record P4(double x, double y, double z, double z_off);
  public enum SurfaceOrientation { Top, Bottom };


  public record ZDepthQueryParameters(double Width, double Length, double Depth, double Percentage, double XMin, double XMax);

  public class SavedZDepthParams
  {

    public string Name { get; set; } = String.Empty;
    public string Site { get; set; } = String.Empty;
    public string Conveyor { get; set; } = String.Empty;
    public double Width { get; set; } = 0;
    public double Length { get; set; } = 0;
    public double Depth { get; set; } = 0;
    public double Percentage { get; set; } = 1;
    public double XMin { get; set; } = 0;
    public double XMax { get; set; } = 0;
  };

  public class Realtime
  {
    public string Site { get; set; } = String.Empty;
    public string Conveyor { get; set; } = String.Empty;
    public DateTime Time { get; set; } = DateTime.Now;
    public double YArea { get; set; } = 0;
    public double Value { get; set; } = 0;
  }

  public class MetaRealtime
  {
    public string Site { get; set; } = String.Empty;
    public string Conveyor { get; set; } = String.Empty;
    public string Id { get; set; } = String.Empty;
    public double Value { get; set; } = 0;
    public bool Valid { get; set; } = false;
  }

  public class SiteName
  {
    public string DisplayName { get; set; } = string.Empty;
    public string Name { get; set; } = string.Empty;
    public string Url { get; set; } = string.Empty;
    public SiteName() { }
    public SiteName(string DisplayName, string Name, string Url) { 
      this.DisplayName = DisplayName;
      this.Name = Name;
      this.Url = Url;
    }
    public void Deconstruct(out string DisplayName, out string Name, out string Url)
    {
      DisplayName = this.DisplayName;
      Name = this.Name;
      Url = this.Url;
    }
  }

  public class AppSettings
  {
    public string NatsUrl { get; set; } = "127.0.0.1";
    public string Authorization { get; set; } = String.Empty;
    public string DBPath { get; set; } = String.Empty;
    public string ConnectionString { get; set; } = String.Empty;
    public bool DoubleSided { get; set; } = false;
    public bool ShowInstructions {get; set;} = false;
    public List<SiteName> Sites { get; set; } = Array.Empty<SiteName>().ToList();
  }

  public sealed class Scan
  {

    public long rowid { get; set; } = 0;
    public string site { get; set; } = String.Empty;
    public string name { get { return NoAsp.EndpointToSQliteDbName(this.site, this.conveyor, this.chrono); } private set { } }
    public string ConveyorID { get { return NoAsp.GetConveyorID(this.site, this.conveyor); } private set { } }
    //public long Revision {get; set;} = 0;
    //public SurfaceOrientation Orientation {get; set;} = SurfaceOrientation.Top;
    //public string FilePath {get; set;} = String.Empty;

    public string conveyor { get; set; } = String.Empty;
    public DateTime chrono { get; set; } = DateTime.UnixEpoch;
    public double x_res { get; set; } = 0;
    public double y_res { get; set; } = 0;
    public double z_res { get; set; } = 0;
    public double z_off { get; set; } = 0;
    public double z_max { get; set; } = 0;
    public double z_min { get; set; } = 0;

    public double Ymax { get; set; } = 0;
    public double YmaxN { get; set; } = 0;

    public double WidthN { get; set; } = 0;
    [NotMapped]
    public double Xmax { get { return WidthN * x_res; } private set { } }
    [NotMapped]
    public double FrameLength { get { return 4000; } private set { } }
    [NotMapped]
    public bool HasData { get { return x_res != 0; } private set { } }
    [NotMapped]
    public (double,double) XBegin{ get { return (-WidthN * x_res / 2, x_res) ; } private set { } }

    public long Belt { get; set; } = 1;
    public int Orientation { get; set; } = -1;

    public Scan(long rowid, string site, string conveyor, DateTime chrono, double x_res, double y_res, double z_res, double z_off, double z_max, double z_min)
    {
      this.rowid = rowid;
      this.site = site;
      this.conveyor = conveyor;
      this.chrono = chrono;
      this.x_res = x_res;
      this.y_res = y_res;
      this.z_res = z_res;
      this.z_off = z_off;
      this.z_max = z_max;
      this.z_min = z_min;
    }

    public Scan(Scan bc)
    {
      rowid = bc.rowid;
      site = bc.site;
      conveyor = bc.conveyor;
      chrono = bc.chrono;
      x_res = bc.x_res;
      y_res = bc.y_res;
      z_res = bc.z_res;
      z_off = bc.z_off;
      z_max = bc.z_max;
      z_min = bc.z_min;
    }

    // Required for ASP ApiController JSON Deserialization 
    public Scan() { }

  }

  public class ClientRect
  {
    public double x { get; set; }
    public double y { get; set; }
    public double bottom { get; set; }
    public double right { get; set; }

    public ClientRect(double X, double Y, double Right, double Bottom)
    {
      x = X;
      y = Y;
      right = Right;
      bottom = Bottom;
    }

    public void Deconstruct(out double X, out double Y, out double Right, out double Bottom)
    {
      X = x;
      Y = y;
      Right = right;
      Bottom = bottom;
    }

  }

  public class Conveyor
  {
    public long Id { get; set; }
    public string Site { get; set; }
    public string Name { get; set; }
    public TimeZoneInfo Timezone { get; set; }
    public double PulleyCircumference { get; set; }
    public long Belt { get; set; }
  }

  public class Belt
  {
    public long Id { get; set; }
    public DateTime Installed { get; set; }
    public double PulleyCover { get; set; }
    public double CordDiameter { get; set; }
    public double TopCover { get; set; }
    public double Length { get; set; }
    public double Width { get; set; }
    public long Splices { get; set; }
    public long Conveyor { get; set; }
  }

  public class PlotInfo
  {
    public double start { get; set; } = 0;
    public double length { get; set; } = 0;
    public double x { get; set; } = 0;
    public string title { get; set; } = String.Empty;


    public PlotInfo(double start, double length, double x, string title)
    {
      this.start = start;
      this.length = length;
      this.x = x;
      this.title = title;
    }



    public void Deconstruct(out double s, out double l, out double i, out string t)
    {
      s = start;
      l = length;
      i = x;
      t = title;
    }

  }

  public static class Lh
  {
    public static IEnumerable<(T item, int index)> WithIndex<T>(this IEnumerable<T> source)
    {
      return source.Select((item, index) => (item, index));
    }
  }
}