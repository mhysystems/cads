using Microsoft.Data.Sqlite;
using CadsFlatbuffers;


namespace cads_gui.Data
{

  class ProfileData : IDisposable
  {
    protected bool disposed = false;
    protected readonly SqliteConnection connection;
    protected readonly SqliteCommand command;

    public ProfileData(string name)
    {

      connection = new SqliteConnection("" +
        new SqliteConnectionStringBuilder
        {
          Mode = SqliteOpenMode.ReadWriteCreate,
          DataSource = name
        });

      connection.Open();

      command = connection.CreateCommand();
      command.Transaction = connection.BeginTransaction();
      command.CommandText = @"CREATE TABLE IF NOT EXISTS Profiles (Y REAL NOT NULL, X REAL NOT NULL, Z BLOB NOT NULL)";
      var sql = command.ExecuteNonQuery();

      command.CommandText = @"INSERT OR REPLACE INTO Profiles (rowid,Y,X,Z) VALUES (@rowid,@Y,@X,@Z)";

      command.Parameters.AddWithValue("@rowid", 1);
      command.Parameters.AddWithValue("@Y", 0.0);
      command.Parameters.AddWithValue("@X", 0.0);
      command.Parameters.AddWithValue("@Z", Array.Empty<float>());

      command.Prepare();
    }

    public void Save(ulong idx, double y, double x, float[] z)
    {

      command.Parameters[0].Value = idx;
      command.Parameters[1].Value = y;
      command.Parameters[2].Value = x;
      command.Parameters[3].Value = NoAsp.ConvertFloatsToBytes(z);
      command.ExecuteNonQuery();

    }

    public void Dispose()
    {
      Dispose(disposing: true);
      GC.SuppressFinalize(this);
    }

    protected virtual void Dispose(bool disposing)
    {

      if (!this.disposed)
      {

        if (disposing)
        {
          command.Transaction?.Commit();
          connection.Dispose();
        }

        disposed = true;
      }
    }
  }

  static class ScanData
  {
    static public void Insert(string name, CadsFlatbuffers.Conveyor conveyor)
    {

      using var connection = new SqliteConnection("" +
        new SqliteConnectionStringBuilder
        {
          Mode = SqliteOpenMode.ReadWriteCreate,
          DataSource = name
        });

      connection.Open();

      var command = connection.CreateCommand();
      command.Transaction = connection.BeginTransaction();
      command.CommandText = @"CREATE TABLE IF NOT EXISTS Conveyor (
        Site TEXT NOT NULL 
        ,Name TEXT NOT NULL 
        ,Timezone TEXT NOT NULL 
        ,PulleyCircumference REAL NOT NULL 
        ,TypicalSpeed REAL NOT NULL 
      )";

      command.ExecuteNonQuery();

      command.CommandText = @"INSERT OR REPLACE INTO Conveyor (
        rowid
        ,Site
        ,Name 
        ,Timezone
        ,PulleyCircumference
        ,TypicalSpeed
      ) VALUES (1,@Site,@Name,@Timezone,@PulleyCircumference,@TypicalSpeed)";

      command.Parameters.AddWithValue("@Site", conveyor.Site);
      command.Parameters.AddWithValue("@Name", conveyor.Name);
      command.Parameters.AddWithValue("@Timezone", conveyor.Timezone);
      command.Parameters.AddWithValue("@PulleyCircumference",conveyor.PulleyCircumference);
      command.Parameters.AddWithValue("@TypicalSpeed",conveyor.TypicalSpeed);

      command.Prepare();
      command.ExecuteNonQuery();
      command.Transaction?.Commit();
    }

    static public void Insert(string name, CadsFlatbuffers.Belt belt)
    {

      using var connection = new SqliteConnection("" +
        new SqliteConnectionStringBuilder
        {
          Mode = SqliteOpenMode.ReadWriteCreate,
          DataSource = name
        });

      connection.Open();

      var command = connection.CreateCommand();
      command.Transaction = connection.BeginTransaction();
      command.CommandText = @"CREATE TABLE IF NOT EXISTS Belt (
        Serial TEXT NOT NULL 
        ,PulleyCover REAL NOT NULL 
        ,CordDiameter REAL NOT NULL 
        ,TopCover REAL NOT NULL 
        ,Length REAL NOT NULL 
        ,Width REAL NOT NULL 
        ,WidthN INTEGER NOT NULL 
      )";

      command.ExecuteNonQuery();

      command.CommandText = @"INSERT OR REPLACE INTO Belt (
        rowid
        ,Serial
        ,PulleyCover 
        ,CordDiameter 
        ,TopCover
        ,Length
        ,Width
        ,WidthN
      ) VALUES (1,@Serial,@PulleyCover,@CordDiameter,@TopCover,@Length,@Width,@WidthN)";

      command.Parameters.AddWithValue("@Serial", belt.Serial);
      command.Parameters.AddWithValue("@PulleyCover", belt.PulleyCover);
      command.Parameters.AddWithValue("@CordDiameter", belt.CordDiameter);
      command.Parameters.AddWithValue("@TopCover",belt.TopCover);
      command.Parameters.AddWithValue("@Length",belt.Length);
      command.Parameters.AddWithValue("@Width",belt.Width);
      command.Parameters.AddWithValue("@WidthN",belt.WidthN);

      command.Prepare();
      command.ExecuteNonQuery();
      command.Transaction?.Commit();
    }

    static public void Insert(string name, CadsFlatbuffers.Meta meta)
    {

      using var connection = new SqliteConnection("" +
        new SqliteConnectionStringBuilder
        {
          Mode = SqliteOpenMode.ReadWriteCreate,
          DataSource = name
        });

      connection.Open();

      var command = connection.CreateCommand();
      command.Transaction = connection.BeginTransaction();
      command.CommandText = @"CREATE TABLE IF NOT EXISTS Meta (
        Version INTEGER NOT NULL 
        ,ZEncoding INTEGER NOT NULL
      )";

      command.ExecuteNonQuery();

      command.CommandText = @"INSERT OR REPLACE INTO Meta (
        rowid
        ,Version
        ,ZEncoding 
      ) VALUES (1,@Version,@ZEncoding)";

      command.Parameters.AddWithValue("@Version", meta.Version);
      command.Parameters.AddWithValue("@ZEncoding", meta.ZEncoding);
 
      command.Prepare();
      command.ExecuteNonQuery();
      command.Transaction?.Commit();
    }
    static public void Insert(string name, CadsFlatbuffers.Limits limits)
    {

      using var connection = new SqliteConnection("" +
        new SqliteConnectionStringBuilder
        {
          Mode = SqliteOpenMode.ReadWriteCreate,
          DataSource = name
        });

      connection.Open();

      var command = connection.CreateCommand();
      command.Transaction = connection.BeginTransaction();
      command.CommandText = @"CREATE TABLE IF NOT EXISTS Limits (
        ZMin REAL NOT NULL 
        ,ZMax REAL NOT NULL 
        ,XMin REAL NOT NULL 
        ,XMax REAL NOT NULL 
      )";

      command.ExecuteNonQuery();

      command.CommandText = @"INSERT OR REPLACE INTO Limits (
        rowid
        ,ZMin 
        ,ZMax 
        ,XMin 
        ,XMax 
      ) VALUES (1,@ZMin,@ZMax,@XMin,@XMax)";

      command.Parameters.AddWithValue("@ZMin", limits.ZMin);
      command.Parameters.AddWithValue("@ZMax", limits.ZMax);
      command.Parameters.AddWithValue("@XMin", limits.XMin);
      command.Parameters.AddWithValue("@XMax", limits.XMax);
 
      command.Prepare();
      command.ExecuteNonQuery();
      command.Transaction?.Commit();
    }

    static public void Insert(string name, CadsFlatbuffers.Gocator gocator)
    {

      using var connection = new SqliteConnection("" +
        new SqliteConnectionStringBuilder
        {
          Mode = SqliteOpenMode.ReadWriteCreate,
          DataSource = name
        });

      connection.Open();

      var command = connection.CreateCommand();
      command.Transaction = connection.BeginTransaction();
      command.CommandText = @"CREATE TABLE IF NOT EXISTS Gocator (
        xResolution REAL NOT NULL
        ,zResolution REAL NOT NULL
        ,zOffset REAL NOT NULL
        ,xOrigin REAL NOT NULL
        ,width REAL NOT NULL
        ,zOrigin REAL NOT NULL
        ,height REAL NOT NULL 
      )";

      command.ExecuteNonQuery();

      command.CommandText = @"INSERT OR REPLACE INTO Gocator (
        rowid
        ,xResolution
        ,zResolution
        ,zOffset
        ,xOrigin
        ,width
        ,zOrigin
        ,height
      ) VALUES (1,@xResolution,@zResolution,@zOffset,@xOrigin,@width,@zOrigin,@height)";

      command.Parameters.AddWithValue("@xResolution", gocator.XResolution);
      command.Parameters.AddWithValue("@zResolution", gocator.ZResolution);
      command.Parameters.AddWithValue("@zOffset", gocator.ZOffset);
      command.Parameters.AddWithValue("@xOrigin", gocator.XOrigin);
      command.Parameters.AddWithValue("@width", gocator.Width);
      command.Parameters.AddWithValue("@zOrigin", gocator.ZOrigin);
      command.Parameters.AddWithValue("@height", gocator.Height);
 
      command.Prepare();
      command.ExecuteNonQuery();
      command.Transaction?.Commit();
    }



  }

}

