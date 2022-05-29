#include "db.h"
#include <nlohmann/json.hpp>

#include <vector>
#include <string>
#include <iostream>

#include <readerwriterqueue.h>
#include <spdlog/spdlog.h>
#include <spdlog/sinks/rotating_file_sink.h>
#include <spdlog/sinks/stdout_color_sinks.h>

#include <fmt/core.h>

using namespace moodycamel;
using json = nlohmann::json;
extern json global_config;

spdlog::logger dblog("db", {std::make_shared<spdlog::sinks::rotating_file_sink_st>("db.log", 1024 * 1024 * 5, 1), std::make_shared<spdlog::sinks::stdout_color_sink_st>()});

namespace cads
{

  using namespace std;

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
            dblog.error("create_db:sqlite3_step Error Code:{}", err);
          }
        }
        else
        {
          dblog.error("create_db:sqlite3_prepare_v2 Error Code:{}", err);
        }

        sqlite3_finalize(stmt);
      }
    }
    else
    {
      dblog.error("create_db:sqlite3_open_v2 Error Code:{}", err);
    }

    sqlite3_close(db);

    return r;
  }

  tuple<sqlite3 *, sqlite3_stmt *> open_db(std::string name)
  {
    sqlite3 *db = nullptr;
    const char *db_name = name.c_str();

    int err = sqlite3_open_v2(db_name, &db, SQLITE_OPEN_READWRITE | SQLITE_OPEN_NOMUTEX | SQLITE_OPEN_SHAREDCACHE, nullptr);

    sqlite3_stmt *stmt = nullptr;
    // auto query = R"(BEGIN TRANSACTION)"s;
    auto query = R"(PRAGMA synchronous=OFF)"s;
    err = sqlite3_prepare_v2(db, query.c_str(), query.size(), &stmt, NULL);
    err = sqlite3_step(stmt);
    query = R"("PRAGMA locking_mode = EXCLUSIVE")"s;
    err = sqlite3_prepare_v2(db, query.c_str(), query.size(), &stmt, NULL);
    err = sqlite3_step(stmt);
    query = R"("PRAGMA journal_mode = MEMORY")"s;
    err = sqlite3_prepare_v2(db, query.c_str(), query.size(), &stmt, NULL);
    err = sqlite3_step(stmt);

    query = R"(INSERT OR REPLACE INTO PROFILE (y,x_off,z) VALUES (?,?,?))"s;
    err = sqlite3_prepare_v2(db, query.c_str(), query.size(), &stmt, NULL);

    return {db, stmt};
  }

  sqlite3_stmt *fetch_profile_statement(sqlite3 *db)
  {
    auto query = R"(SELECT * FROM PROFILE WHERE y=?)"s;
    sqlite3_stmt *stmt = nullptr;
    auto err = sqlite3_prepare_v2(db, query.c_str(), query.size(), &stmt, NULL);
    return stmt;
  }

  profile fetch_profile(sqlite3_stmt *stmt, y_type y_p)
  {
    int err;

    if constexpr (std::is_same_v<y_type, double>)
    {
      err = sqlite3_bind_double(stmt, 1, (double)y_p);
    }
    else
    {
      err = sqlite3_bind_int64(stmt, 1, (int64_t)y_p);
    }

    err = sqlite3_step(stmt);

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
      z_type zv{z, z + len};
      // err = sqlite3_step(stmt);
      sqlite3_reset(stmt);
      return {y, x_off, zv};
    }
    else
    {
      sqlite3_reset(stmt);
      return {std::numeric_limits<y_type>::max(), NAN, {}};
    }
  }

  std::tuple<double, double, double, double, double> fetch_profile_parameters(std::string name)
  {

    sqlite3 *db = nullptr;
    sqlite3_stmt *stmt = nullptr;

    const char *db_name = name.c_str();
    int err = sqlite3_open_v2(db_name, &db, SQLITE_OPEN_READONLY, nullptr);

    auto query = R"(SELECT * FROM PARAMETERS)"s;
    err = sqlite3_prepare_v2(db, query.c_str(), query.size(), &stmt, NULL);

    err = sqlite3_step(stmt);

    std::tuple<double, double, double, double, double> rtn;
    if (err == SQLITE_ROW)
    {

      rtn = {
          sqlite3_column_double(stmt, 1),
          sqlite3_column_double(stmt, 2),
          sqlite3_column_double(stmt, 3),
          sqlite3_column_double(stmt, 4),
          sqlite3_column_double(stmt, 5)};
    }
    else
    {
      rtn = {1.0, 1.0, 1.0, 1.0, 1.0};
    }

    if (stmt != nullptr)
      sqlite3_finalize(stmt);
    if (db != nullptr)
      sqlite3_close(db);

    return rtn;
  }

  void store_profile_parameters(double y_res, double x_res, double z_res, double z_off, double encoder_res)
  {
    sqlite3 *db = nullptr;
    sqlite3_stmt *stmt = nullptr;

    const char *db_name = global_config["db_name"].get<std::string>().c_str();
    int err = sqlite3_open_v2(db_name, &db, SQLITE_OPEN_READWRITE, nullptr);

    auto query = R"(INSERT OR REPLACE INTO PARAMETERS (row_id,y_res,x_res,z_res,z_off,encoder_res) VALUES (0,?,?,?,?,?))"s;
    err = sqlite3_prepare_v2(db, query.c_str(), query.size(), &stmt, NULL);
    err = sqlite3_bind_double(stmt, 1, y_res);
    err = sqlite3_bind_double(stmt, 2, x_res);
    err = sqlite3_bind_double(stmt, 3, z_res);
    err = sqlite3_bind_double(stmt, 4, z_off);
    err = sqlite3_bind_double(stmt, 5, encoder_res);

    err = SQLITE_BUSY;

    while (err == SQLITE_BUSY)
    {
      err = sqlite3_step(stmt);
      std::this_thread::sleep_for(std::chrono::milliseconds(50));
    }

    if (stmt != nullptr)
      sqlite3_finalize(stmt);
    if (db != nullptr)
      sqlite3_close(db);
  }

  void store_profile_thread(BlockingReaderWriterQueue<profile> &db_fifo)
  {

    auto log = spdlog::rotating_logger_st("db", "db.log", 1024 * 1024 * 5, 1);

    sqlite3 *db = nullptr;
    const char *db_name = global_config["db_name"].get<std::string>().c_str();

    int err = sqlite3_open_v2(db_name, &db, SQLITE_OPEN_READWRITE | SQLITE_OPEN_NOMUTEX | SQLITE_OPEN_SHAREDCACHE, nullptr);

    sqlite3_stmt *stmt = nullptr;
    auto query = R"(PRAGMA synchronous=OFF)"s;
    err = sqlite3_prepare_v2(db, query.c_str(), query.size(), &stmt, NULL);
    err = sqlite3_step(stmt);
    query = R"("PRAGMA locking_mode = EXCLUSIVE")"s;
    err = sqlite3_prepare_v2(db, query.c_str(), query.size(), &stmt, NULL);
    err = sqlite3_step(stmt);
    query = R"("PRAGMA journal_mode = MEMORY")"s;
    err = sqlite3_prepare_v2(db, query.c_str(), query.size(), &stmt, NULL);
    err = sqlite3_step(stmt);
    query = R"("PRAGMA temp_store = MEMORY")"s;
    err = sqlite3_prepare_v2(db, query.c_str(), query.size(), &stmt, NULL);
    err = sqlite3_step(stmt);
    query = R"("PRAGMA mmap_size = 10000000")"s;
    err = sqlite3_prepare_v2(db, query.c_str(), query.size(), &stmt, NULL);
    err = sqlite3_step(stmt);
    query = R"("PRAGMA page_size = 32768")"s;
    err = sqlite3_prepare_v2(db, query.c_str(), query.size(), &stmt, NULL);
    err = sqlite3_step(stmt);
    query = R"(INSERT OR REPLACE INTO PROFILE (y,x_off,z) VALUES (?,?,?))"s;
    err = sqlite3_prepare_v2(db, query.c_str(), query.size(), &stmt, NULL);

    while (true)
    {

      log->flush();
      profile p;
      db_fifo.wait_dequeue(p);

      if (p.y == std::numeric_limits<y_type>::max())
        break;

      if constexpr (std::is_same_v<y_type, double>)
      {
        err = sqlite3_bind_double(stmt, 1, (double)p.y);
      }
      else
      {
        err = sqlite3_bind_int64(stmt, 1, (int64_t)p.y);
      }

      err = sqlite3_bind_double(stmt, 2, p.x_off);
      err = sqlite3_bind_blob(stmt, 3, p.z.data(), p.z.size() * sizeof(int16_t), SQLITE_STATIC);

      err = sqlite3_step(stmt);
      auto attempts = 512;

      while (err != SQLITE_DONE && attempts-- > 0)
      {
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
        err = sqlite3_step(stmt);
      }

      if (err != SQLITE_DONE)
      {
        log->error("SQLite Error Code:{}", err);
      }

      sqlite3_reset(stmt);
    }

    if (stmt != nullptr)
      sqlite3_finalize(stmt);
    if (db != nullptr)
      sqlite3_close(db);
  }

  coro<y_type, profile> store_profile_coro(profile p)
  {

    auto log = spdlog::rotating_logger_st("db", "db.log", 1024 * 1024 * 5, 1);

    sqlite3 *db = nullptr;
    const char *db_name = global_config["db_name"].get<std::string>().c_str();

    int err = sqlite3_open_v2(db_name, &db, SQLITE_OPEN_READWRITE | SQLITE_OPEN_NOMUTEX | SQLITE_OPEN_SHAREDCACHE, nullptr);

    sqlite3_stmt *stmt = nullptr;
    auto query = R"(PRAGMA synchronous=OFF)"s;
    err = sqlite3_prepare_v2(db, query.c_str(), query.size(), &stmt, NULL);
    err = sqlite3_step(stmt);
    query = R"("PRAGMA locking_mode = EXCLUSIVE")"s;
    err = sqlite3_prepare_v2(db, query.c_str(), query.size(), &stmt, NULL);
    err = sqlite3_step(stmt);
    query = R"("PRAGMA journal_mode = MEMORY")"s;
    err = sqlite3_prepare_v2(db, query.c_str(), query.size(), &stmt, NULL);
    err = sqlite3_step(stmt);
    query = R"("PRAGMA temp_store = MEMORY")"s;
    err = sqlite3_prepare_v2(db, query.c_str(), query.size(), &stmt, NULL);
    err = sqlite3_step(stmt);
    query = R"("PRAGMA mmap_size = 10000000")"s;
    err = sqlite3_prepare_v2(db, query.c_str(), query.size(), &stmt, NULL);
    err = sqlite3_step(stmt);
    query = R"("PRAGMA page_size = 32768")"s;
    err = sqlite3_prepare_v2(db, query.c_str(), query.size(), &stmt, NULL);
    err = sqlite3_step(stmt);
    query = R"(INSERT OR REPLACE INTO PROFILE (y,x_off,z) VALUES (?,?,?))"s;
    err = sqlite3_prepare_v2(db, query.c_str(), query.size(), &stmt, NULL);

    while (true)
    {

      if constexpr (std::is_same_v<y_type, double>)
      {
        err = sqlite3_bind_double(stmt, 1, (double)p.y);
      }
      else
      {
        err = sqlite3_bind_int64(stmt, 1, (int64_t)p.y);
      }
      err = sqlite3_bind_double(stmt, 2, p.x_off);
      err = sqlite3_bind_blob(stmt, 3, p.z.data(), p.z.size() * sizeof(int16_t), SQLITE_STATIC);

      err = sqlite3_step(stmt);
      auto attempts = 512;

      while (err != SQLITE_DONE && attempts-- > 0)
      {
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
        err = sqlite3_step(stmt);
      }

      if (err != SQLITE_DONE)
      {
        log->error("SQLite Error Code:{}", err);
      }

      sqlite3_reset(stmt);

      p = co_yield p.y;
      if (p.y == std::numeric_limits<decltype(p.y)>::max())
        break;
    }

    if (stmt != nullptr)
      sqlite3_finalize(stmt);
    if (db != nullptr)
      sqlite3_close(db);

    co_return std::numeric_limits<decltype(p.y)>::max();
  }

  bool store_profile(sqlite3_stmt *stmt, const profile &p)
  {
    int err;
    if constexpr (std::is_same_v<decltype(p.y), double>)
    {
      err = sqlite3_bind_double(stmt, 1, (double)p.y);
    }
    else
    {
      err = sqlite3_bind_int64(stmt, 1, (int64_t)p.y);
    }

    err = sqlite3_bind_double(stmt, 2, p.x_off);
    err = sqlite3_bind_blob(stmt, 3, p.z.data(), p.z.size() * sizeof(z_element), SQLITE_STATIC);

    err = sqlite3_step(stmt);

    sqlite3_reset(stmt);

    return err == SQLITE_DONE;
  }

  void close_db(sqlite3 *db, sqlite3_stmt *stmt, sqlite3_stmt *stmt2)
  {

    if (stmt != nullptr)
      sqlite3_finalize(stmt);
    if (stmt2 != nullptr)
      sqlite3_finalize(stmt2);
    if (db != nullptr)
      sqlite3_close(db);
  }

}
