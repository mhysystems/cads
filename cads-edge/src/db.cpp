#include "db.h"

#include <vector>
#include <string>
#include <iostream>
#include <filesystem>
#include <memory>

#include <fmt/core.h>
#include <spdlog/spdlog.h>

#include <readerwriterqueue.h>
#include <constants.h>

using namespace moodycamel;

namespace cads
{

  using namespace std;
  using sqlite3_t = std::unique_ptr<sqlite3, decltype(&sqlite3_close)>;
  using sqlite3_stmt_t = std::unique_ptr<sqlite3_stmt, decltype(&sqlite3_finalize)>;

  tuple<sqlite3_t, sqlite3_stmt_t> prepare_query(string name, string query, int flags = SQLITE_OPEN_READONLY | SQLITE_OPEN_NOMUTEX)
  {

    sqlite3 *db_raw = nullptr;
    auto db_config_name = name.empty() ? global_config["db_name"].get<std::string>() : name;
    const char *db_name = db_config_name.c_str();
    auto err = sqlite3_open_v2(db_name, &db_raw, flags, nullptr);

    if (err != SQLITE_OK)
    {
      std::throw_with_nested(std::runtime_error("prepare_querysqlite3_open_v2"));
    }

    sqlite3_t db(db_raw, &sqlite3_close);

    sqlite3_stmt *stmt_raw = nullptr;
    err = sqlite3_prepare_v2(db.get(), query.c_str(), (int)query.size(), &stmt_raw, NULL);

    if (err != SQLITE_OK)
    {
      std::throw_with_nested(std::runtime_error("prepare_query:sqlite3_prepare_v2"));
    }

    sqlite3_stmt_t stmt(stmt_raw, &sqlite3_finalize);

    return {move(db), move(stmt)};
  }

  tuple<int, sqlite3_stmt_t> db_step(sqlite3_stmt_t stmt)
  {
    auto err = sqlite3_step(stmt.get());
    auto attempts = 512;

    while (err != SQLITE_ROW && err != SQLITE_DONE && attempts-- > 0)
    {
      std::this_thread::sleep_for(std::chrono::milliseconds(10));
      err = sqlite3_step(stmt.get());
    }

    return {err, move(stmt)};
  }

  bool create_db(std::string name)
  {
    sqlite3 *db = nullptr;
    bool r = true;
    auto db_name_string = name.empty() ? global_config["db_name"].get<std::string>() : name;
    const char *db_name = db_name_string.c_str();

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
        R"(PRAGMA journal_mode=WAL)"s,
        R"(DROP TABLE IF EXISTS PROFILE;)"s,
        R"(DROP TABLE IF EXISTS PARAMETERS;)"s,
        R"(VACUUM;)"s,
        fmt::format(R"(CREATE TABLE IF NOT EXISTS PROFILE (revid INTEGER NOT NULL, idx INTEGER NOT NULL,y {} NOT NULL, x_off REAL NOT NULL, z BLOB NOT NULL, PRIMARY KEY (revid,idx));)", ytype),
        R"(CREATE TABLE IF NOT EXISTS PARAMETERS (y_res REAL NOT NULL, x_res REAL NOT NULL, z_res REAL NOT NULL, z_off REAL NOT NULL, encoder_res REAL NOT NULL, z_max REAL NOT NULL))"s};

    std::filesystem::remove(name);
    std::filesystem::remove(name + "-shm");
    std::filesystem::remove(name + "-wal");
    int err = sqlite3_open_v2(db_name, &db, SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE, nullptr);

    if (err == SQLITE_OK)
    {

      for (auto query : tables)
      {

        char *errmsg;
        err = sqlite3_exec(db, query.c_str(), nullptr, nullptr, &errmsg);

        if (err != SQLITE_OK)
        {
          spdlog::get("db")->error("create_db:sqlite3_exec, Error Code:{}, query:{}, sqlite error msg:{}", err, query, errmsg);
          sqlite3_free(errmsg);
        }
      }
    }
    else
    {
      spdlog::get("db")->error("create_db:sqlite3_open_v2 Error Code:{}", err);
      std::throw_with_nested(std::runtime_error("create_db:sqlite3_open_v2"));
    }

    sqlite3_close(db);

    return r;
  }

  int store_profile_parameters(profile_params params, std::string name)
  {
    auto query = R"(INSERT OR REPLACE INTO PARAMETERS (rowid,y_res,x_res,z_res,z_off,encoder_res,z_max) VALUES (1,?,?,?,?,?,?))"s;
    auto [db, stmt] = prepare_query(name, query, SQLITE_OPEN_READWRITE | SQLITE_OPEN_NOMUTEX);

    auto err = sqlite3_bind_double(stmt.get(), 1, params.y_res);
    err = sqlite3_bind_double(stmt.get(), 2, params.x_res);
    err = sqlite3_bind_double(stmt.get(), 3, params.z_res);
    err = sqlite3_bind_double(stmt.get(), 4, params.z_off);
    err = sqlite3_bind_double(stmt.get(), 5, params.encoder_res);
    err = sqlite3_bind_double(stmt.get(), 6, params.z_max);

    tie(err, stmt) = db_step(move(stmt));

    if (err != SQLITE_DONE)
    {
      spdlog::get("db")->error("store_profile_parameters: SQLite Error Code:{}", err);
    }

    return err;
  }

  std::tuple<profile_params, int> fetch_profile_parameters(std::string name)
  {

    auto query = R"(SELECT * FROM PARAMETERS WHERE ROWID = 1)"s;
    auto [db, stmt] = prepare_query(name, query);
    auto err = SQLITE_OK;

    tie(err, stmt) = db_step(move(stmt));

    std::tuple<profile_params, int> rtn;
    if (err == SQLITE_ROW)
    {

      rtn = {
          {sqlite3_column_double(stmt.get(), 0),
           sqlite3_column_double(stmt.get(), 1),
           sqlite3_column_double(stmt.get(), 2),
           sqlite3_column_double(stmt.get(), 3),
           sqlite3_column_double(stmt.get(), 4),
           sqlite3_column_double(stmt.get(), 5)},
          0};
    }
    else
    {
      rtn = {{1.0, 1.0, 1.0, 0.0, 1.0, 1.0}, -1};
    }

    return rtn;
  }

  std::tuple<double, double, double, double, int> fetch_belt_dimensions(int revid, std::string name)
  {

    auto query = R"(SELECT MIN(Y),MAX(Y),COUNT(Y),LENGTH(Z) FROM PROFILE WHERE REVID = ? LIMIT 1)"s;
    auto [db, stmt] = prepare_query(name, query);

    auto err = sqlite3_bind_int(stmt.get(), 1, revid);

    tie(err, stmt) = db_step(move(stmt));

    std::tuple<double, double, double, double, int> rtn;
    if (err == SQLITE_ROW)
    {

      if (sqlite3_column_double(stmt.get(), 0) != 0.0)
      {
        spdlog::get("db")->error("Sanity check failed. Minimum Y is not 0 but {}", sqlite3_column_double(stmt.get(), 0));
      }

      rtn = {
          sqlite3_column_double(stmt.get(), 0),
          sqlite3_column_double(stmt.get(), 1),
          sqlite3_column_double(stmt.get(), 2) - 1, // Sqlite indexes from 1 not 0
          sqlite3_column_double(stmt.get(), 3) / sizeof(cads::z_element),
          0};
    }
    else
    {
      rtn = {0.0, 0.0, 0.0, 0.0, -1};
    }

    return rtn;
  }

  long count_with_width_n(string name, int revid, int width_n) {
    auto query = fmt::format(R"(SELECT count(idx) FROM PROFILE WHERE REVID = {} AND LENGTH(z) = ?)", revid);
    auto [db, stmt] = prepare_query(name, query);
    auto err = sqlite3_bind_int(stmt.get(), 1, width_n);

    tie(err, stmt) = db_step(move(stmt));

    if(err != SQLITE_ROW) {
      spdlog::get("db")->error("count_with_width_n: sqlite err: {}",err);
      return -1;
    }

    auto cnt = sqlite3_column_int64(stmt.get(), 0);
    return cnt;
  }

  coro<int, std::tuple<int, int, profile>, 1> store_profile_coro(std::string name)
  {

    auto query = R"(INSERT OR REPLACE INTO PROFILE (revid,idx,y,x_off,z) VALUES (?,?,?,?,?))"s;
    auto [db, stmt] = prepare_query(name, query, SQLITE_OPEN_READWRITE | SQLITE_OPEN_NOMUTEX);

    auto query2 = R"(PRAGMA synchronous=OFF)"s;
    char *errmsg;
    auto err = sqlite3_exec(db.get(), query2.c_str(), nullptr, nullptr, &errmsg);

    if (err != SQLITE_OK)
    {
      spdlog::get("db")->error("create_db:sqlite3_exec, Error Code:{}, query:{}, sqlite error msg:{}", err, query, errmsg);
      sqlite3_free(errmsg);
    }

    while (true)
    {
      auto [data, terminate] = co_yield err;

      if (terminate)
        break;

      auto [rev, idx, p] = data;

      if (p.y == std::numeric_limits<decltype(p.y)>::max())
        break;

      err = sqlite3_bind_int(stmt.get(), 1, rev);
      err = sqlite3_bind_int(stmt.get(), 2, idx);

      if constexpr (std::is_same_v<y_type, double>)
      {
        err = sqlite3_bind_double(stmt.get(), 3, p.y);
      }
      else
      {
        err = sqlite3_bind_int64(stmt.get(), 3, (int64_t)p.y);
      }
      err = sqlite3_bind_double(stmt.get(), 4, p.x_off);

      auto n = p.z.size() * sizeof(z_element);
      if (n > std::numeric_limits<int>::max())
      {
        std::throw_with_nested(std::runtime_error("z data size greater than sqlite limits"));
      }
      err = sqlite3_bind_blob(stmt.get(), 5, p.z.data(), (int)n, SQLITE_STATIC);

      // Run once, retrying not effective, too slow and causes buffers to fill
      err = sqlite3_step(stmt.get());

      sqlite3_reset(stmt.get());

      if (err != SQLITE_DONE)
      {
        spdlog::get("db")->error("SQLite Error Code:{}, revid:{}, idx:{}", err, rev, idx);
      }
    }

    spdlog::get("db")->info("store_profile_coro finished");
    co_return err;
  }

  std::tuple<std::deque<std::tuple<int, profile>>, sqlite3_stmt_t> fetch_belt_coro_step(sqlite3_stmt_t stmt, int idx_begin, int idx_end)
  {

    std::deque<std::tuple<int, profile>> rtn;
    auto err = sqlite3_bind_int(stmt.get(), 1, idx_begin);
    err = sqlite3_bind_int(stmt.get(), 2, idx_end);

    while (true)
    {
      // Run once, retrying not effective, too slow and causes buffers to fill
      err = sqlite3_step(stmt.get());

      if (err == SQLITE_ROW)
      {
        double y;

        auto idx = sqlite3_column_int(stmt.get(), 0);

        y = sqlite3_column_double(stmt.get(), 1);

        double x_off = sqlite3_column_double(stmt.get(), 2);
        z_element *z = (z_element *)sqlite3_column_blob(stmt.get(), 3); // Freed on next call to sqlite3_step
        int len = sqlite3_column_bytes(stmt.get(), 3) / sizeof(*z);

        rtn.push_back({idx, {y, x_off, {z, z + len}}});
      }
      else if (err == SQLITE_DONE)
      {
        break;
      }
      else
      {
        spdlog::get("db")->error("fetch_belt_coro_step:sqlite3_step Code:{}", err);
        break;
      }
    }

    sqlite3_reset(stmt.get());

    return {rtn, std::move(stmt)};
  }

  coro<std::tuple<int, profile>> fetch_belt_coro(int revid, int last_idx, int size, std::string name)
  {
    auto query = fmt::format(R"(SELECT idx,y,x_off,z FROM PROFILE WHERE REVID = {} AND IDX >= ? AND IDX < ? ORDER BY Y)", revid);
    auto [db, stmt] = prepare_query(name, query);

    for (int i = 0; i < last_idx; i += size)
    {
      auto iend = i + size; 
      if(iend > last_idx) iend = last_idx;
      
      auto [p, s] = fetch_belt_coro_step(std::move(stmt), i, iend);
      stmt = std::move(s);

      if (p.empty())
        break;

      while (!p.empty())
      {
        auto [ignore, terminate] = co_yield p.front();
        p.pop_front();
        if (terminate)
        {
          i = last_idx;
        }
      }
    }
  }

}
