using System.Collections.Generic;
using System.Linq;
using System.ComponentModel.DataAnnotations.Schema;
using System;
using NATS.Client.JetStream;

namespace cads_gui.Data
{
  public record P4(double x, double y, double z, double z_off);
  public record ScanLimits(double Width, long WidthN, double Length, long LengthN, double ZMin, double ZMax, double XMin = 0);
  public enum SurfaceOrientation { Top, Bottom };


  public record ZDepthQueryParameters(double Width, double Length, double ZMax, double Percentage, double XMin, double XMax, double ZMin);

  public class SavedZDepthParams
  {

    public string Name { get; set; } = String.Empty;
    public string Site { get; set; } = String.Empty;
    public string Conveyor { get; set; } = String.Empty;
    public double Width { get; set; } = 0;
    public double Length { get; set; } = 0;
    public double ZMax { get; set; } = 0;
    public double Percentage { get; set; } = 1;
    public double XMin { get; set; } = 0;
    public double XMax { get; set; } = 0;
    public double ZMin { get; set; } = 0;
  };

  public class AnomalyMsg
  {
    public string Measurement { get; set; } = String.Empty;
    public string Site { get; set; } = String.Empty;
    public string Conveyor { get; set; } = String.Empty;
    public int Quality { get; set; } = -1;
    public int Revision { get; set; } = -1;
    public double Value { get; set; } = 0;
    public double Location { get; set; } = 0;
    public DateTime Timestamp { get; set; } = DateTime.UtcNow;
  }

  public class MeasureMsg
  {
    public string Measurement { get; set; } = String.Empty;
    public string Site { get; set; } = String.Empty;
    public string Conveyor { get; set; } = String.Empty;
    public int Quality { get; set; } = -1;
    public int Revision { get; set; } = -1;
    public double Value { get; set; } = 0;
    public DateTime Timestamp { get; set; } = DateTime.UtcNow;
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
    public string InfluxDB { get; set; } = "http://127.0.0.1:8086";
    public string InfluxAuth { get; set; } = String.Empty;
    public string NatsUrl { get; set; } = "127.0.0.1";
    public string Authorization { get; set; } = String.Empty;
    public string DBPath { get; set; } = String.Empty;
    public string ConnectionString { get; set; } = String.Empty;
    public bool DoubleSided { get; set; } = false;
    public bool ShowInstructions {get; set;} = false;
    public int NumDisplayLines {get; set;} = 1024;
    public int FrameLength {get; set;} = 4000;
    public List<SiteName> Sites { get; set; } = Array.Empty<SiteName>().ToList();
  }

  public sealed class Scan
  {
    public long Id { get; set; } = 0;
    public long ConveyorId {get; set;} = 0;
    public long BeltId { get; set; } = 0;
    public DateTime Chrono { get; set; } = DateTime.UnixEpoch;
    public long Status { get; set; } = 0;
    public string Filepath { get; set; } = String.Empty;


    // Required for ASP ApiController JSON Deserialization 
    //public Scan() { }

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
    public long Id { get; set; } = 0;
    public string Site { get; set; } = string.Empty;
    public string Name { get; set; } = string.Empty;
    public TimeZoneInfo Timezone { get; set; } = TimeZoneInfo.Utc;
    public double PulleyCircumference { get; set; } = 0;
    public double TypicalSpeed { get; set; } = 0;
  }

  public class Belt
  {
    public long Id { get; set; } = 0;
    public string Serial { get; set; } = String.Empty;
    public double PulleyCover { get; set; } = 0;
    public double CordDiameter { get; set; } = 0;
    public double TopCover { get; set; } = 0;
    public double Length { get; set; } = 0;
    public double Width { get; set; } = 0;

  }

  public class BeltInstall
  {
    public long Id { get; set; } = 0;
    public long ConveyorId { get; set; } = 0;
    public long BeltId { get; set; } = 0;
    public DateTime Chrono { get; set; } = DateTime.UnixEpoch;
  }

  public class Grafana 
  {
    public int Id { get; set;} = 1;
    public string Url { get; set;} = "";
    public int Row { get; set;} = 0;
    public int Col { get; set;} = 0;
    public bool Visible { get; set;} = false;
    public long Belt { get; set; } = 0;
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