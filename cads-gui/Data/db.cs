using System;
using Microsoft.Data.Sqlite;
using System.Linq;

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
          command.Transaction?.Commit();
          connection.Dispose();
        }

        disposed = true;
      }
    }
  }

}

