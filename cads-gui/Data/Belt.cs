using System.Collections.Generic;
using System.Linq;
using System.ComponentModel.DataAnnotations.Schema;
using System;

namespace cads_gui.Data
{
		public record P4(double x, double y, double z, double z_off);

    public record ZDepthQueryParameters(double Width, double Length, double Depth, double Percentage); 

		public class SavedZDepthParams{

    	[DatabaseGenerated(DatabaseGeneratedOption.Identity)]
      public long rowid {get; set;}
			public string Name {get;set;}
      public string Site {get; set;}
      public string Conveyor {get;set;}
			public double Width {get; set;}
			public double Length {get; set;}
		  public double Depth {get; set;}
			public double Percentage {get; set;}
		};

    public class Realtime {
      public string Site {get; set;}
      public string Conveyor {get; set;}
      public DateTime Time {get; set;}
      public double YArea {get; set;}
      public double Value {get; set;}
    }

    public class MetaRealtime {
      public string Site {get; set;}
      public string Conveyor {get; set;}
      public string Id {get; set;}
      public double Value {get; set;}
    }

    public class AppSettings
    {
        public string NatsUrl { get; set; } = "127.0.0.1";
    }

		public class Belt {
    	[DatabaseGenerated(DatabaseGeneratedOption.Identity)]
			public long rowid {get; set;}
			public string site {get; set;}
      [NotMapped]
      public string name {get{return NoAsp.EndpointToSQliteDbName(this.site,this.conveyor,this.chrono);} private set{}}
      [NotMapped]
      public string ConveyorID {get{return NoAsp.GetConveyorID(this.site,this.conveyor);} private set{}}

			public string conveyor {get; set;}
      public DateTime chrono {get; set;}
			public double x_res { get; set; }
      public double y_res { get; set; }
      public double z_res { get; set; }
			public double z_off { get; set; }
			public double z_max { get; set; }
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
	  public class Ranomaly
    {

        public double minX { get; set; }
        public double minY { get; set; }
        public double maxX { get; set; }
        public double maxY { get; set; }
        public double minZ { get; set; }
        public double volume { get; set; }
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
			public string Site { get; set; } = "";
			public string Belt { get; set; } = "";
			public string Category {get; set;} = "";
			public string Flow {get; set;} = "";
			public int Cads {get; set;} = 0;
		}

		public class LiveAnomaly
    {
        public ulong Id { get; set; } = 0;
        public string Category { get; set; } = "";
        public string Severity { get; set; } = "";
        public DateTime Date {get; set;} = DateTime.Now;
				public string Location { get; set; } = "";
        public string Anomaly { get; set; } = "";
        public string Status {get; set;} = "";

				public LiveAnomaly() {}
        public LiveAnomaly(ulong id, string category, string severity, DateTime date, string location, string anomaly, string status) {

					this.Id = id;
					this.Category = category;
					this.Severity = severity;
					this.Date =	date;		
					this.Location = location;
					this.Anomaly = anomaly;
					this.Status = status; 

				}

        public void Deconstruct(out ulong id, out string category, out string severity, out DateTime date, out string location, out string anomaly, out string status)
        {
					id = this.Id;
					category = this.Category;
					severity = this.Severity;
					date = this.Date;		
					location = this.Location;
					anomaly = this.Anomaly;
					status = this.Status; 
        }
    }

    public class PlotInfo
    {
        public double start { get; set; } = 0;
        public double length { get; set; } = 0;
        public double x { get; set; } = 0;
        public string title {get; set;} = "";


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

    public class BeltOld
    {

        public  int anomaly_ID { get; set; }
        public  string category { get; set; }
        public  int danger { get; set; }
        public  string time { get; set; }
        public  double area { get; set; }
        public  double z_depth { get; set; }
        public  double x_lower { get; set; }
        public  double x_upper { get; set; }
        public  double volume { get; set; }
        public  double start { get; set; }
        public  double length { get; set; }
        public  double contour_x { get; set; }
        public  double contour_theta { get; set; }
        public  int epoch { get; set; }
        public  string comment { get; set; }
 
    }
    public class Chart 
    {
        public  int anomaly_ID { get; set; }
        public  bool visible { get; set; }
    }

    public class BeltChart
    {
      public BeltOld belt;
      public bool visible;
      public BeltChart(BeltOld b, bool v)  {
        belt = b;
        visible = v;
      }
    }


		public static class Lh {
      public static IEnumerable<(T item, int index)> WithIndex<T>(this IEnumerable<T> source)
      {
          return source.Select((item, index) => (item, index));
      }
    }
}