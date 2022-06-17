#include "db.h"

#include <vector>
#include <string>
#include <iostream>

#include <fmt/core.h>
#include <spdlog/spdlog.h>
#include <spdlog/sinks/rotating_file_sink.h>
#include <spdlog/sinks/stdout_color_sinks.h>

#include <readerwriterqueue.h>
#include <constants.h>


using namespace moodycamel;

namespace cads
{

  using namespace std;

  int db_step(sqlite3_stmt *stmt)
  {
    auto err = sqlite3_step(stmt);
    auto attempts = 512;

    while (err != SQLITE_ROW && err != SQLITE_DONE && attempts-- > 0)
    {
      std::this_thread::sleep_for(std::chrono::milliseconds(10));
      err = sqlite3_step(stmt);
    }

    return err;
  }

  bool create_db(std::string name)
  {
    sqlite3 *db = nullptr;
    bool r = true;
    const char *db_name = name.c_str();

    string ytype;

    if constexpr (std::is_same_v<y_type, double>)
    {
      ytype = "REAL";
    }
    else
    {
      ytype = "INTEGER";
    }

    vector<string> tables{
        R"(DROP TABLE IF EXISTS PROFILE;)"s,
        R"(VACUUM;)"s,
        fmt::format(R"(CREATE TABLE IF NOT EXISTS PROFILE (y {} PRIMARY KEY, x_off REAL NOT NULL, z BLOB NOT NULL);)", ytype),
        R"(CREATE TABLE IF NOT EXISTS PARAMETERS (row_id INTEGER PRIMARY KEY,  y_res REAL NOT NULL, x_res REAL NOT NULL, z_res REAL NOT NULL, z_off REAL NOT NULL, encoder_res REAL NOT NULL);)"s
        };

    int err = sqlite3_open_v2(db_name, &db, SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE, nullptr);

    if (err == SQLITE_OK)
    {

      sqlite3_stmt *stmt = nullptr;
      for (auto query : tables)
      {

        err = sqlite3_prepare_v2(db, query.c_str(), query.size(), &stmt, NULL);
        if (err == SQLITE_OK)
        {
          err = sqlite3_step(stmt);
          if (err != SQLITE_DONE)
          {
            r &= false;
            spdlog::get("db")->error("create_db:sqlite3_step Error Code:{}", err);
          }
        }
        else
        {
          spdlog::get("db")->error("create_db:sqlite3_prepare_v2 Error Code:{}", err);
        }

        sqlite3_finalize(stmt);
      }
    }
    else
    {
      spdlog::get("db")->error("create_db:sqlite3_open_v2 Error Code:{}", err);
    }

    sqlite3_close(db);

    return r;
  }

  int store_profile_parameters(double y_res, double x_res, double z_res, double z_off, double encoder_res)
  {
    sqlite3 *db = nullptr;
    sqlite3_stmt *stmt = nullptr;

    const char *db_name = global_config["db_name"].get<std::string>().c_str();
    int err = sqlite3_open_v2(db_name, &db, SQLITE_OPEN_READWRITE | SQLITE_OPEN_NOMUTEX | SQLITE_OPEN_SHAREDCACHE, nullptr);

    auto query = R"(INSERT OR REPLACE INTO PARAMETERS (row_id,y_res,x_res,z_res,z_off,encoder_res) VALUES (0,?,?,?,?,?))"s;
    err = sqlite3_prepare_v2(db, query.c_str(), query.size(), &stmt, NULL);
    err = sqlite3_bind_double(stmt, 1, y_res);
    err = sqlite3_bind_double(stmt, 2, x_res);
    err = sqlite3_bind_double(stmt, 3, z_res);
    err = sqlite3_bind_double(stmt, 4, z_off);
    err = sqlite3_bind_double(stmt, 5, encoder_res);

    err = db_step(stmt);

    sqlite3_finalize(stmt);
    sqlite3_close(db);

    if (err != SQLITE_DONE)
    {
      spdlog::get("db")->error("SQLite Error Code:{}", err);
    }

    return err;
  }

  void prepare_step_query(sqlite3 *db, std::string query)
  {
    sqlite3_stmt *stmt = nullptr;
    auto err = sqlite3_prepare_v2(db, query.c_str(), query.size(), &stmt, NULL);

    if (err != SQLITE_OK)
    {
      std::throw_with_nested(std::runtime_error(query));
    }

    err = sqlite3_step(stmt);
    auto attempts = 512;

    while (err != SQLITE_DONE && attempts-- > 0)
    {
      sqlite3_reset(stmt);
      std::this_thread::sleep_for(std::chrono::milliseconds(10));
      err = sqlite3_step(stmt);
    }

    if (err != SQLITE_DONE)
    {
      std::throw_with_nested(std::runtime_error(query));
    }

    sqlite3_finalize(stmt);
  }

  
  coro<int, profile> store_profile_coro(profile p)
  {

    sqlite3 *db = nullptr;
    const char *db_name = global_config["db_name"].get<std::string>().c_str();

    int err = sqlite3_open_v2(db_name, &db, SQLITE_OPEN_READWRITE | SQLITE_OPEN_NOMUTEX, nullptr);

    sqlite3_stmt *stmt = nullptr;
    auto query = R"(PRAGMA synchronous=OFF)"s;
    prepare_step_query(db, query);

    query = R"(INSERT OR REPLACE INTO PROFILE (y,x_off,z) VALUES (?,?,?))"s;
    err = sqlite3_prepare_v2(db, query.c_str(), query.size(), &stmt, NULL);

    while (true)
    {

      if constexpr (std::is_same_v<y_type, double>)
      {
        err = sqlite3_bind_double(stmt, 1, (double)std::round(p.y));
      }
      else
      {
        err = sqlite3_bind_int64(stmt, 1, (int64_t)p.y);
      }
      err = sqlite3_bind_double(stmt, 2, p.x_off);
      err = sqlite3_bind_blob(stmt, 3, p.z.data(), p.z.size() * sizeof(z_element), SQLITE_STATIC);

      err = db_step(stmt);
      sqlite3_reset(stmt);
      
      if (err != SQLITE_DONE)
      {
        spdlog::get("db")->error("SQLite Error Code:{}", err);
        break;
      }

      p = co_yield err;
      if (p.y == std::numeric_limits<decltype(p.y)>::max())
        break;
    }

    sqlite3_finalize(stmt);
    sqlite3_close(db);
  }

  std::tuple<double, double, double, double, double, int> fetch_profile_parameters(std::string name)
  {

    sqlite3 *db = nullptr;
    sqlite3_stmt *stmt = nullptr;

    const char *db_name = name.c_str();
    int err = sqlite3_open_v2(db_name, &db, SQLITE_OPEN_READONLY | SQLITE_OPEN_NOMUTEX, nullptr);

    auto query = R"(SELECT * FROM PARAMETERS)"s;
    err = sqlite3_prepare_v2(db, query.c_str(), query.size(), &stmt, NULL);

    err = db_step(stmt);

    std::tuple<double, double, double, double, double, int> rtn;
    if (err == SQLITE_ROW)
    {

      rtn = {
          sqlite3_column_double(stmt, 1),
          sqlite3_column_double(stmt, 2),
          sqlite3_column_double(stmt, 3),
          sqlite3_column_double(stmt, 4),
          sqlite3_column_double(stmt, 5),
          0};
    }
    else
    {
      rtn = {1.0, 1.0, 1.0, 0.0, 1.0, 0};
    }

    sqlite3_finalize(stmt);
    sqlite3_close(db);

    return rtn;
  }

  coro<std::tuple<profile, int>> fetch_belt_coro()
  {

    sqlite3 *db = nullptr;
    const char *db_name = global_config["db_name"].get<std::string>().c_str();

    int err = sqlite3_open_v2(db_name, &db, SQLITE_OPEN_READONLY | SQLITE_OPEN_NOMUTEX | SQLITE_OPEN_SHAREDCACHE, nullptr);

    sqlite3_stmt *stmt = nullptr;
    auto query = R"(SELECT * FROM PROFILE ORDER BY Y)"s;

    err = sqlite3_prepare_v2(db, query.c_str(), query.size(), &stmt, NULL);

    if constexpr (std::is_same_v<y_type, double>)
    {
      err = sqlite3_bind_double(stmt, 1, (double)0);
    }
    else
    {
      err = sqlite3_bind_int64(stmt, 1, (int64_t)0);
    }

    while (true)
    {
      std::tuple<profile,int> p;
      err = db_step(stmt);

      if (err == SQLITE_ROW)
      {
        y_type y;

        if constexpr (std::is_same_v<y_type, double>)
        {
          y = (y_type)sqlite3_column_double(stmt, 0);
        }
        else
        {
          y = (y_type)sqlite3_column_int64(stmt, 0);
        }

        double x_off = sqlite3_column_double(stmt, 1);
        z_element *z = (z_element *)sqlite3_column_blob(stmt, 2);
        int len = sqlite3_column_bytes(stmt, 2) / sizeof(*z);

        p =  {{y, x_off, {z, z + len}}, 0};
      }
      else if (err == SQLITE_DONE)
      {
        break;
      }
      else
      {
        spdlog::get("db")->error("fetch_profile:sqlite3_step Code:{}", err);
        break;
      }
      
      co_yield p;
    }

    sqlite3_finalize(stmt);
    sqlite3_close(db);
  }

}
