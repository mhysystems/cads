using System.Collections.Generic;
using System.Linq;
using System.ComponentModel.DataAnnotations.Schema;
using System;

namespace cads_gui.Data
{
		public record P4(double x, double y, double z, double z_off);

    public record ZDepthQueryParameters(double Width, double Length, double Depth, double Percentage, double XMin, double XMax); 

		public class SavedZDepthParams{

    	[DatabaseGenerated(DatabaseGeneratedOption.Identity)]
      public long rowid {get; set;} = 0;
			public string Name {get;set;} = String.Empty;
      public string Site {get; set;} = String.Empty;
      public string Conveyor {get;set;} = String.Empty;
			public double Width {get; set;} = 0;
			public double Length {get; set;} = 0;
		  public double Depth {get; set;} = 0;
			public double Percentage {get; set;} = 0;
		};

    public class Realtime {
      public string Site {get; set;} = String.Empty;
      public string Conveyor {get; set;} = String.Empty;
      public DateTime Time {get; set;} = DateTime.Now;
      public double YArea {get; set;} = 0;
      public double Value {get; set;} = 0;
    }

    public class MetaRealtime {
      public string Site {get; set;} = String.Empty;
      public string Conveyor {get; set;} = String.Empty;
      public string Id {get; set;} = String.Empty;
      public double Value {get; set;} = 0;
      public bool Valid {get; set;} = false;
    }

    public class AppSettings
    {
        public string NatsUrl { get; set; } = "127.0.0.1";
        public string Authorization { get; set; } = String.Empty;
        public string DBPath {get; set;} = String.Empty;
    }

		public class Belt {
    	[DatabaseGenerated(DatabaseGeneratedOption.Identity)]
			public long rowid {get; set;} = 0;
			public string site {get; set;} = String.Empty;
      [NotMapped]
      public string name {get{return NoAsp.EndpointToSQliteDbName(this.site,this.conveyor,this.chrono);} private set{}}
      [NotMapped]
      public string ConveyorID {get{return NoAsp.GetConveyorID(this.site,this.conveyor);} private set{}}

			public string conveyor {get; set;} = String.Empty;
      public DateTime chrono {get; set;} = DateTime.Now;
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
      public double Xmax { get{return WidthN * x_res;} private set{} }
      [NotMapped]
      public double FrameLength { get{return 4000;} private set{} }

			public Belt(long rowid, string site, string conveyor, DateTime chrono, double x_res, double y_res, double z_res, double z_off, double z_max) {
          this.rowid = rowid;
          this.site = site;
          this.conveyor = conveyor;
          this.chrono = chrono;
          this.x_res = x_res;
          this.y_res = y_res;
          this.z_res = z_res;
          this.z_off = z_off;
          this.z_max = z_max;
      }

      public Belt(Belt bc) {
        rowid  = bc.rowid;
        site   = bc.site;
        conveyor   = bc.conveyor;
        chrono = bc.chrono;
        x_res  = bc.x_res;
        y_res  = bc.y_res;
        z_res  = bc.z_res;
        z_off  = bc.z_off;
        z_max  = bc.z_max;
      }

      // Required for JSON Deserialization
      public Belt() {}

		}

    public class ClientRect
    {
        public double x { get; set; }
        public double y { get; set; }
        public double bottom { get; set; }
        public double right { get; set; }

        public ClientRect(double X, double Y, double Right, double Bottom) {
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

		public class Conveyors 
		{
			[DatabaseGenerated(DatabaseGeneratedOption.Identity)]
			public long rowid {get; set;}
			public string Site { get; set; } = String.Empty;
			public string Belt { get; set; } = String.Empty;
			public string Category {get; set;} = String.Empty;
			public string Flow {get; set;} = String.Empty;
			public int Cads {get; set;} = 0;
		}

		public class PlotInfo
    {
        public double start { get; set; } = 0;
        public double length { get; set; } = 0;
        public double x { get; set; } = 0;
        public string title {get; set;} = String.Empty;


        public PlotInfo(double start, double length, double x, string title) {
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

		public static class Lh {
      public static IEnumerable<(T item, int index)> WithIndex<T>(this IEnumerable<T> source)
      {
          return source.Select((item, index) => (item, index));
      }
    }
}