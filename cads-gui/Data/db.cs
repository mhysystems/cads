using System;
using Microsoft.Data.Sqlite;
using System.Linq;

namespace cads_gui.Data
{

  public static class NoEFCore
  {
    public static IEnumerable<(DateTime,string)> RetrieveConveyorScanAsync(string site, string conveyor, string db = "conveyors.db")
    {

      using var connection = new SqliteConnection("" +
        new SqliteConnectionStringBuilder
        {
          Mode = SqliteOpenMode.ReadOnly,
          DataSource = db
        });

      connection.Open();
      var query = $"select chrono from BELTINFO where site = @site and conveyor = @conveyor order by chrono";
      var command = connection.CreateCommand();
      command.CommandText = query;
      command.Parameters.AddWithValue("@site", site);
      command.Parameters.AddWithValue("@conveyor", conveyor);
      command.Prepare();


      using var reader = command.ExecuteReader();

      List<(DateTime,string)> rtn = new();

      while (reader.Read())
      {
        var chronos = reader.GetString(0);
        var chrono = DateTime.Parse(chronos);

        rtn.Add((chrono,NoAsp.EndpointToSQliteDbName(site, conveyor, chrono)));

      }

      return rtn.AsEnumerable();

    }
  }

  class ProfileData : IDisposable
  {
    protected bool disposed = false;
    protected SqliteConnection connection;
    protected SqliteCommand command;

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
      command.Parameters[3].Value = NoAsp.f2b(z);
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
          command.Transaction.Commit();
          connection.Dispose();
        }

        disposed = true;
      }
    }
  }

}

