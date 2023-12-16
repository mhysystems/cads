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
      command.CommandText = @"CREATE TABLE IF NOT EXISTS PROFILE (y REAL NOT NULL, x_off REAL NOT NULL, z BLOB NOT NULL)";
      var sql = command.ExecuteNonQuery();

      command.CommandText = @"INSERT OR REPLACE INTO PROFILE (rowid,y,x_off,z) VALUES (@rowid,@y,@x_off,@z)";

      command.Parameters.AddWithValue("@rowid", 1);
      command.Parameters.AddWithValue("@y", 0.0);
      command.Parameters.AddWithValue("@x_off", 0.0);
      command.Parameters.AddWithValue("@z", Array.Empty<float>());

      command.Prepare();
    }

    public void Save(ulong idx, double y, double x_off, float[] z)
    {

      command.Parameters[0].Value = idx;
      command.Parameters[1].Value = y;
      command.Parameters[2].Value = x_off;
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
    static public void Insert(string name, conveyor conveyor)
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

    static public void Insert(string name, belt belt)
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
  }

}

