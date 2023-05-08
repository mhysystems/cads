#include "db.h"

#include <vector>
#include <string>
#include <iostream>
#include <filesystem>
#include <memory>
#include <sstream>

#include <fmt/core.h>
#include <fmt/chrono.h>
#include <spdlog/spdlog.h>

#include <readerwriterqueue.h>
#include <constants.h>
#include <utils.hpp>

using namespace moodycamel;
using namespace std::string_literals;

using sqlite3_t = std::unique_ptr<sqlite3, decltype(&sqlite3_close)>;
using sqlite3_stmt_t = std::unique_ptr<sqlite3_stmt, decltype(&sqlite3_finalize)>;

namespace
{
  auto db_exec(sqlite3* db,  std::string query)
  {
    std::string func_name = __func__;
    int err = SQLITE_ERROR;

    for(int retry = 2; retry > 0; --retry) {

        char *errmsg;
        err = sqlite3_exec(db, query.c_str(), nullptr, nullptr, &errmsg);

        if (err != SQLITE_OK)
        {
          spdlog::get("cads")->error("{}: sqlite3_exec error code:{}, query:{}, sqlite error msg:{}", func_name, err, query, errmsg);
          sqlite3_free(errmsg);
        }

        retry = 0;
    }

    return err;
  }

  auto db_exec(std::string db_name_string, std::string query)
  {
    std::string func_name = __func__;

    sqlite3 *db = nullptr;

    const char *db_name = db_name_string.c_str();

    auto err = sqlite3_open_v2(db_name, &db, SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE, nullptr);

    if (err == SQLITE_OK)
    {
      err = db_exec(db, query.c_str());
    }
    else
    {
      spdlog::get("cads")->error("{}: sqlite3_open_v2({}) error code:{}", func_name, db_name_string, err);
    }

    sqlite3_close(db);

    return err;
  }

  auto db_exec(std::string db_name, std::vector<std::string> queries)
  {
    auto err = SQLITE_ERROR;

    for (auto query : queries)
    {

      err = db_exec(db_name, query);

      if (err != SQLITE_OK)
      {
        break;
      }
    }

    return err;
  }
  
  int sqlite3_prepare_retry
  (
  sqlite3 *db,
  const char *zSql,
  int nByte,
  sqlite3_stmt **ppStmt,
  const char **pzTail,
  int retry = 16
)
  {
        int err = SQLITE_ERROR;

    for(; retry > 0; --retry) {

        err = sqlite3_prepare_v2(db, zSql, nByte, ppStmt, pzTail);

        if (err != SQLITE_OK)
        {
          std::this_thread::sleep_for(std::chrono::milliseconds(10));
          
        }else {
          retry = 0;
        }
    }

    return err;
  }

  std::tuple<sqlite3_stmt_t,sqlite3_t> prepare_query(std::string db_config_name, std::string query, int flags = SQLITE_OPEN_READONLY | SQLITE_OPEN_NOMUTEX)
  {

    sqlite3 *db_raw = nullptr;
    const char *db_name = db_config_name.c_str();
    auto err = sqlite3_open_v2(db_name, &db_raw, flags, nullptr);

    if (err != SQLITE_OK)
    {
      auto errmsg = sqlite3_errstr(err);
      std::throw_with_nested(std::runtime_error(fmt::format("{} : sqlite3_open_v2 {},{}",__func__,err, errmsg)));
    }

    sqlite3_t db(db_raw, &sqlite3_close);

    sqlite3_stmt *stmt_raw = nullptr;
    err = sqlite3_prepare_retry(db.get(), query.c_str(), (int)query.size(), &stmt_raw, NULL);

    if (err != SQLITE_OK)
    {
      auto errmsg = sqlite3_errstr(err);
      std::throw_with_nested(std::runtime_error(fmt::format("{} : sqlite3_prepare_retry {},{}",__func__,err, errmsg)));
    }

    sqlite3_stmt_t stmt(stmt_raw, &sqlite3_finalize);

    // Undefined behaviour. FIXME Current destruction order is left to right
    return {move(stmt),move(db)};
  }

  std::tuple<int, sqlite3_stmt_t> db_step(sqlite3_stmt_t stmt)
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

}

namespace cads
{

  int store_daily_upload(date::utc_clock::time_point datetime, std::string name)
  {
    auto query = R"(UPDATE STATE SET DAILYUPLOAD = ? WHERE ROWID = 1)"s;
    auto db_config_name = name.empty() ? global_config["state_db_name"].get<std::string>() : name;
    auto [stmt,db] = prepare_query(db_config_name, query, SQLITE_OPEN_READWRITE | SQLITE_OPEN_NOMUTEX);
    
    auto ts = to_str(datetime);

    auto err = sqlite3_bind_text(stmt.get(), 1, ts.c_str(),ts.size(),nullptr);

    if (err != SQLITE_OK)
    {
      spdlog::get("db")->error("store_daily_upload: SQLite Error Code:{}", err);
    }

    tie(err, stmt) = db_step(move(stmt));

    if (err != SQLITE_DONE)
    {
      spdlog::get("db")->error("db_step: SQLite Error Code:{}", err);
    }

    return err;
  }

  date::utc_clock::time_point fetch_daily_upload(std::string name)
  {
    auto query = R"(SELECT DAILYUPLOAD FROM STATE WHERE ROWID = 1)"s;
    auto db_config_name = name.empty() ? global_config["state_db_name"].get<std::string>() : name;
    auto [stmt,db] = prepare_query(db_config_name, query);
    auto err = SQLITE_OK;

    tie(err, stmt) = db_step(move(stmt));
    
    if (err != SQLITE_ROW)
    {
      spdlog::get("db")->error("db_step: SQLite Error Code:{}", err);
    }
    
    const char* date_cstr = (const char* )sqlite3_column_text(stmt.get(), 0);
    auto date_cstr_len = sqlite3_column_bytes(stmt.get(), 0);

    std::string date_str(date_cstr,date_cstr_len);
    return to_clk(date_str); 
  }

  
  void store_conveyor_id(int id, std::string name)
  {
    auto query = R"(UPDATE STATE SET ConveyorId = ? WHERE ROWID = 1)"s;
    auto db_config_name = name.empty() ? global_config["state_db_name"].get<std::string>() : name;
    auto [stmt,db] = prepare_query(db_config_name, query, SQLITE_OPEN_READWRITE | SQLITE_OPEN_NOMUTEX);
    
    auto err = sqlite3_bind_int(stmt.get(), 1, id);

    if (err != SQLITE_OK)
    {
      spdlog::get("db")->error("store_conveyor_id: SQLite Error Code:{}", err);
    }

    tie(err, stmt) = db_step(move(stmt));

    if (err != SQLITE_DONE)
    {
      spdlog::get("db")->error("db_step: SQLite Error Code:{}", err);
    }
  }

  std::tuple<int,bool> fetch_conveyor_id(std::string name)
  {
    auto query = R"(SELECT ConveyorId FROM STATE WHERE ROWID = 1)"s;
    auto db_config_name = name.empty() ? global_config["state_db_name"].get<std::string>() : name;
    auto [stmt,db] = prepare_query(db_config_name, query);
    auto err = SQLITE_OK;

    tie(err, stmt) = db_step(move(stmt));
    
    if (err != SQLITE_ROW)
    {
      spdlog::get("db")->error("db_step: SQLite Error Code:{}", err);
      return {0,true};
    }
    
    int r = sqlite3_column_int(stmt.get(), 0);

    return {r,false}; 
  }

  void store_belt_id(int id, std::string name)
  {
    auto query = R"(UPDATE STATE SET BeltId = ? WHERE ROWID = 1)"s;
    auto db_config_name = name.empty() ? global_config["state_db_name"].get<std::string>() : name;
    auto [stmt,db] = prepare_query(db_config_name, query, SQLITE_OPEN_READWRITE | SQLITE_OPEN_NOMUTEX);
    
    auto err = sqlite3_bind_int(stmt.get(), 1, id);

    if (err != SQLITE_OK)
    {
      spdlog::get("db")->error("store_conveyor_id: SQLite Error Code:{}", err);
    }

    tie(err, stmt) = db_step(move(stmt));

    if (err != SQLITE_DONE)
    {
      spdlog::get("db")->error("db_step: SQLite Error Code:{}", err);
    }
  }

  void store_scan_uploaded(int64_t idx, std::string scan_name, std::string name)
  {
    auto query = R"(update scans set uploaded = ? where rowid = (select rowid from scans where Path=?);)"s;
    auto db_config_name = name.empty() ? global_config["state_db_name"].get<std::string>() : name;
    auto [stmt,db] = prepare_query(db_config_name, query, SQLITE_OPEN_READWRITE | SQLITE_OPEN_NOMUTEX);
    
    auto err = sqlite3_bind_int(stmt.get(), 1, idx);
    err = sqlite3_bind_text(stmt.get(), 2, scan_name.c_str(), scan_name.size(), nullptr);

    if (err != SQLITE_OK)
    {
      spdlog::get("db")->error("store_conveyor_id: SQLite Error Code:{}", err);
    }

    tie(err, stmt) = db_step(move(stmt));

    if (err != SQLITE_DONE)
    {
      spdlog::get("db")->error("db_step: SQLite Error Code:{}", err);
    }
  }

  void store_scan_status(int64_t status, std::string scan_name, std::string name)
  {
    auto query = R"(update scans set status = ? where rowid = (select rowid from scans where Path=?);)"s;
    auto db_config_name = name.empty() ? global_config["state_db_name"].get<std::string>() : name;
    auto [stmt,db] = prepare_query(db_config_name, query, SQLITE_OPEN_READWRITE | SQLITE_OPEN_NOMUTEX);
    
    auto err = sqlite3_bind_int(stmt.get(), 1, status);
    err = sqlite3_bind_text(stmt.get(), 2, scan_name.c_str(), scan_name.size(), nullptr);

    if (err != SQLITE_OK)
    {
      spdlog::get("db")->error("store_conveyor_id: SQLite Error Code:{}", err);
    }

    tie(err, stmt) = db_step(move(stmt));

    if (err != SQLITE_DONE)
    {
      spdlog::get("db")->error("db_step: SQLite Error Code:{}", err);
    }
  }

  std::tuple<int64_t,bool> fetch_scan_uploaded(std::string scan_name, std::string name)
  {
    auto query = R"(select uploaded from scans where Path=?)"s;
    auto db_config_name = name.empty() ? global_config["state_db_name"].get<std::string>() : name;
    auto [stmt,db] = prepare_query(db_config_name, query);
    auto err = sqlite3_bind_text(stmt.get(), 1, scan_name.c_str(), scan_name.size(), nullptr);

    tie(err, stmt) = db_step(move(stmt));
    
    if (err != SQLITE_ROW)
    {
      spdlog::get("db")->error("db_step: SQLite Error Code:{}", err);
      return {0,true};
    }
    
    int64_t r = sqlite3_column_int64(stmt.get(), 0);

    return {r,false}; 
  }

  std::tuple<int,bool> fetch_belt_id(std::string name)
  {
    auto query = R"(SELECT BeltId FROM STATE WHERE ROWID = 1)"s;
    auto db_config_name = name.empty() ? global_config["state_db_name"].get<std::string>() : name;
    auto [stmt,db] = prepare_query(db_config_name, query);
    auto err = SQLITE_OK;

    tie(err, stmt) = db_step(move(stmt));
    
    if (err != SQLITE_ROW)
    {
      spdlog::get("db")->error("db_step: SQLite Error Code:{}", err);
      return {0,true};
    }
    
    int r = sqlite3_column_int(stmt.get(), 0);

    return {r,false}; 
  }

  
  void create_profile_db(std::string name = ""s)
  {
    auto db_name = name.empty() ? global_config["profile_db_name"].get<std::string>() : name;

    std::vector<std::string> tables{
        R"(PRAGMA journal_mode=WAL)"s,
        R"(DROP TABLE IF EXISTS PROFILE;)"s,
        R"(DROP TABLE IF EXISTS PARAMETERS;)"s,
        R"(CREATE TABLE IF NOT EXISTS PROFILE (revid INTEGER NOT NULL, idx INTEGER NOT NULL,y REAL NOT NULL, x_off REAL NOT NULL, z BLOB NOT NULL, PRIMARY KEY (revid,idx));)"s,
        R"(CREATE TABLE IF NOT EXISTS PARAMETERS (y_res REAL NOT NULL, x_res REAL NOT NULL, z_res REAL NOT NULL, z_off REAL NOT NULL, encoder_res REAL NOT NULL, z_max REAL NOT NULL))"s
        };
    
    if(global_config["startup_delete_db"].get<bool>()) {
      std::filesystem::remove(db_name);
      std::filesystem::remove(db_name + "-shm");
      std::filesystem::remove(db_name + "-wal");
    }

    auto err = db_exec(db_name,tables);
    
    if(err != SQLITE_OK) {
      std::throw_with_nested(std::runtime_error("create_profile_db"));
    }

  }

  void create_program_state_db(std::string name = ""s)
  {
    using namespace date;
    using namespace std::chrono;

    auto sts = global_config["daily_start_time"].get<std::string>();
    auto db_name = name.empty() ? global_config["state_db_name"].get<std::string>() : name;
    
    std::string ts = "";

    if (sts != "now"s)
    {
      std::stringstream st{sts};

      system_clock::duration run_in;
      st >> parse("%R", run_in);

      date::zoned_time now{ date::current_zone(), std::chrono::system_clock::now()};
      auto trigger = std::chrono::floor<std::chrono::days>(now.get_local_time()) + run_in;
      
      date::zoned_time trigger_local{ date::current_zone(), trigger};
      auto trigger_utc = date::make_zoned("UTC", trigger_local);
      ts = date::format("%FT%TZ", trigger_utc);

    }
    else
    {
      ts = to_str(date::utc_clock::now());
    }

    std::vector<std::string> tables{
      R"(PRAGMA journal_mode=WAL)"s,
      R"(CREATE TABLE IF NOT EXISTS STATE (DAILYUPLOAD TEXT NOT NULL, ConveyorId INTEGER NOT NULL, BeltId INTEGER NOT NULL))",
      R"(CREATE TABLE IF NOT EXISTS SCANS (ScanBegin TEXT NOT NULL, Path TEXT NOT NULL, Uploaded INTEGER NOT NULL, Status INTEGER NOT NULL))",
      fmt::format(R"(INSERT INTO STATE(DAILYUPLOAD,ConveyorId,BeltId) SELECT '{}',{},{} WHERE NOT EXISTS (SELECT * FROM STATE))", ts,0,0),
      R"(VACUUM)"
    };

    auto err = db_exec(db_name,tables);
    
    if(err != SQLITE_OK) {
      std::throw_with_nested(std::runtime_error("create_program_state_db"));
    }

  }

  void create_transient_db(std::string name = ""s)
  {
    using namespace date;
    using namespace std::chrono;

    auto db_name = name.empty() ? global_config["transient_db_name"].get<std::string>() : name;
    
    std::vector<std::string> tables{
      R"(PRAGMA journal_mode=WAL)"s,
      R"(CREATE TABLE IF NOT EXISTS Transients (LastY REAL NOT NULL))",
      R"(CREATE TABLE IF NOT EXISTS ErroredProfile (Id TEXT, z BLOB NOT NULL))",
      fmt::format(R"(INSERT INTO Transients(LastY) SELECT {} WHERE NOT EXISTS (SELECT * FROM Transients))",-1.0)
    };


    auto err = db_exec(db_name,tables);

    if(err != SQLITE_OK)
    {
      std::throw_with_nested(std::runtime_error("create_transient_db"));
    }
  }

  void create_default_dbs() {
    create_profile_db();
    create_program_state_db();
    create_transient_db();
  }

  int store_profile_parameters(profile_params params, std::string name)
  {
    auto query = R"(INSERT OR REPLACE INTO PARAMETERS (rowid,y_res,x_res,z_res,z_off,encoder_res,z_max) VALUES (1,?,?,?,?,?,?))"s;
    auto db_config_name = name.empty() ? global_config["profile_db_name"].get<std::string>() : name;
    auto [stmt,db] = prepare_query(db_config_name, query, SQLITE_OPEN_READWRITE | SQLITE_OPEN_NOMUTEX);

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

  coro<int, double,1> store_last_y_coro(std::string name)
  {

    auto query =  R"(UPDATE Transients set LastY = ? where rowid = 1)"s;
    auto db_config_name = name.empty() ? global_config["transient_db_name"].get<std::string>() : name;
    auto [stmt,db] = prepare_query(db_config_name, query, SQLITE_OPEN_READWRITE | SQLITE_OPEN_NOMUTEX);

    char *errmsg = nullptr;
    auto err = sqlite3_exec(db.get(), R"(PRAGMA synchronous=OFF)"s.c_str(), nullptr, nullptr, &errmsg);

    if (err != SQLITE_OK)
    {
      spdlog::get("db")->error("store_last_y_coro:sqlite3_exec, Error Code:{}, query:{}, sqlite error msg:{}", err, query, errmsg);
      sqlite3_free(errmsg);
    }

    while (true)
    {
      auto [data, terminate] = co_yield err;

      if (terminate)
        break;

      auto y = data;

      if (y == std::numeric_limits<decltype(y)>::max())
        break;

      err = sqlite3_bind_double(stmt.get(), 1, y);

      // Run once, retrying not effective, too slow and causes buffers to fill
      err = sqlite3_step(stmt.get());

      sqlite3_reset(stmt.get());

      if (err != SQLITE_DONE)
      {
        spdlog::get("db")->error("{} : sqlite error ({},{})", __func__, err,sqlite3_errstr(err));
      }

    }

    spdlog::get("db")->info("store_last_y finished");
    co_return err;
  }

  void store_errored_profile(const cads::z_type &z, std::string id, std::string name)
  {
    auto query = R"(INSERT INTO ErroredProfile (Id,z) VALUES (?,?))"s;
    auto db_config_name = name.empty() ? global_config["transient_db_name"].get<std::string>() : name;
    auto [stmt,db] = prepare_query(db_config_name, query, SQLITE_OPEN_READWRITE | SQLITE_OPEN_NOMUTEX);
    
    sqlite3_exec(db.get(), R"(PRAGMA synchronous=OFF)"s.c_str(), nullptr, nullptr, nullptr);
    
    sqlite3_bind_text(stmt.get(), 1, id.c_str(), id.size(),nullptr);
    auto n = z.size() * sizeof(z_element);
    sqlite3_bind_blob(stmt.get(), 2, z.data(), (int)n, SQLITE_STATIC);

    sqlite3_step(stmt.get());
  }

  std::tuple<profile_params, int> fetch_profile_parameters(std::string name)
  {

    auto query = R"(SELECT * FROM PARAMETERS WHERE ROWID = 1)"s;
    auto db_config_name = name.empty() ? global_config["profile_db_name"].get<std::string>() : name;
    auto [stmt,db] = prepare_query(db_config_name, query);
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


  std::tuple<double, double, double, double, int> fetch_belt_dimensions(int revid, int idx, std::string name)
  {

    auto query = R"(SELECT MIN(Y),MAX(Y),COUNT(Y),LENGTH(Z) FROM PROFILE WHERE REVID = ? AND IDX < ? LIMIT 1)"s;
    auto db_config_name = name.empty() ? global_config["profile_db_name"].get<std::string>() : name;
    auto [stmt,db] = prepare_query(db_config_name, query);

    auto err = sqlite3_bind_int(stmt.get(), 1, revid);
    err = sqlite3_bind_int(stmt.get(), 2, idx);

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

  long count_with_width_n(std::string name, int revid, int width_n) {
    auto query = fmt::format(R"(SELECT count(idx) FROM PROFILE WHERE REVID = {} AND LENGTH(z) = ?)", revid);
    auto db_config_name = name.empty() ? global_config["profile_db_name"].get<std::string>() : name;
    auto [stmt,db] = prepare_query(db_config_name, query);
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
    auto db_config_name = name.empty() ? global_config["profile_db_name"].get<std::string>() : name;
    auto [stmt,db] = prepare_query(db_config_name, query, SQLITE_OPEN_READWRITE | SQLITE_OPEN_NOMUTEX);

    char *errmsg = nullptr;
    auto err = sqlite3_exec(db.get(), R"(PRAGMA synchronous=OFF)"s.c_str(), nullptr, nullptr, &errmsg);

    if (err != SQLITE_OK)
    {
      spdlog::get("db")->error("store_profile_coro:sqlite3_exec, Error Code:{}, query:{}, sqlite error msg:{}", err, query, errmsg);
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

  std::tuple<std::deque<std::tuple<int, z_type>>, sqlite3_stmt_t> fetch_scan_coro_step(sqlite3_stmt_t stmt, int rowid_begin, int rowid_end)
  {

    std::deque<std::tuple<int, z_type>> rtn;
    auto err = sqlite3_bind_int(stmt.get(), 1, rowid_begin);
    err = sqlite3_bind_int(stmt.get(), 2, rowid_end);

    while (true)
    {
      // Run once, retrying not effective, too slow and causes buffers to fill
      err = sqlite3_step(stmt.get());

      if (err == SQLITE_ROW)
      {

        auto rowid = sqlite3_column_int(stmt.get(), 0);

        z_element *z = (z_element *)sqlite3_column_blob(stmt.get(), 1); // Freed on next call to sqlite3_step
        int len = sqlite3_column_bytes(stmt.get(), 1) / sizeof(*z);

        rtn.push_back({rowid, {z, z + len}});
      }
      else if (err == SQLITE_DONE)
      {
        break;
      }
      else
      {
        spdlog::get("db")->error("{}:sqlite3_step Code:{}",__func__, err);
        break;
      }
    }

    sqlite3_reset(stmt.get());

    return {rtn, std::move(stmt)};
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

  coro<std::tuple<int, profile>> fetch_belt_coro(int revid, long last_idx, long first_index, int size, std::string name)
  {
    auto query = fmt::format(R"(SELECT idx,y,x_off,z FROM PROFILE WHERE REVID = {} AND IDX >= ? AND IDX < ?)", revid);
    auto db_config_name = name.empty() ? global_config["profile_db_name"].get<std::string>() : name;
    auto [stmt,db] = prepare_query(db_config_name, query);

    for (long i = first_index; i < last_idx; i += size)
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

  long max_rowid(std::string table, std::string db_name) {

    auto query = fmt::format(R"(SELECT max(rowid) FROM {})", table);
    
    auto [stmt,db] = prepare_query(db_name, query);

    int err = SQLITE_ERROR;

    tie(err, stmt) = db_step(move(stmt));

    if(err != SQLITE_ROW) {
      spdlog::get("cads")->error("{} : error code {}",__func__, err);
      return -1;
    }

    auto cnt = sqlite3_column_int64(stmt.get(), 0);
    return cnt;
  }

  long zs_count(std::string db_name) 
  {
    return max_rowid("ZS",db_name);
  }


  std::deque<state::scan> fetch_scan_state(std::string name)
  {

    auto query = R"(SELECT ScanBegin, Path, Uploaded, Status FROM Scans)"s;
    auto db_config_name = name.empty() ? global_config["state_db_name"].get<std::string>() : name;
    auto [stmt,db] = prepare_query(db_config_name, query);
    
    std::deque<state::scan>  rtn;
    
    int err = SQLITE_ERROR;

    do{
      
      tie(err, stmt) = db_step(move(stmt));

      if (err == SQLITE_ROW)
      {
        
        auto tmp = std::make_tuple(
          to_clk(std::string( (const char* )sqlite3_column_text(stmt.get(), 0), sqlite3_column_bytes(stmt.get(), 0))),
          std::string( (const char* )sqlite3_column_text(stmt.get(), 1), sqlite3_column_bytes(stmt.get(), 1)),
          int64_t(sqlite3_column_int64(stmt.get(), 2)),
          int64_t(sqlite3_column_int64(stmt.get(), 3))
        );

        rtn.push_back(tmp); 
      }
    }while(err == SQLITE_ROW);
    
    return rtn;
  }

  bool store_scan_state(std::string scan_db, std::string db_name)
  {
    auto query = R"(INSERT INTO SCANS (ScanBegin, Path, Uploaded, Status) VALUES(?,?,?,?))"s;
    auto db_config_name = db_name.empty() ? global_config["state_db_name"].get<std::string>() : db_name;
    auto [stmt,db] = prepare_query(db_config_name, query, SQLITE_OPEN_READWRITE | SQLITE_OPEN_NOMUTEX);
    
    auto ScanBegin = date::format("%FT%TZ",std::chrono::floor<std::chrono::seconds>(date::utc_clock::now()));
    auto Path = scan_db;

    auto err = sqlite3_bind_text(stmt.get(), 1, ScanBegin.c_str(), ScanBegin.size(),nullptr);
    err = sqlite3_bind_text(stmt.get(), 2, Path.c_str(), Path.size(),nullptr);
    err = sqlite3_bind_int64(stmt.get(), 3, 0L);
    err = sqlite3_bind_int64(stmt.get(), 4, 0L);

    tie(err, stmt) = db_step(move(stmt));

    return err == SQLITE_OK || err == SQLITE_DONE;
  }

  bool delete_scan_state(std::string Path, std::string db_name)
  {
    auto query = R"(DELETE FROM Scans WHERE Path = ?)"s;
    auto db_config_name = db_name.empty() ? global_config["state_db_name"].get<std::string>() : db_name;
    auto [stmt,db] = prepare_query(db_config_name, query, SQLITE_OPEN_READWRITE | SQLITE_OPEN_NOMUTEX);
    
    auto err = sqlite3_bind_text(stmt.get(), 1, Path.c_str(), Path.size(),nullptr);


    tie(err, stmt) = db_step(move(stmt));

    return err == SQLITE_OK || err == SQLITE_DONE;
  }


  // scan db

  bool store_scan_gocator(std::tuple<double,double,double,double> gocator, std::string db_name) 
  {
    
    auto err = db_exec(db_name, R"(CREATE TABLE IF NOT EXISTS GOCATOR(ZRes REAL NOT NULL, ZOff REAL NOT NULL, ZCard REAL NOT NULL, Framerate REAL NOT NULL))"s);
    
    auto query = R"(INSERT INTO GOCATOR (ZRes, ZOff, ZCard, Framerate) VALUES (?,?,?,?))"s;

    auto [stmt,db] = prepare_query(db_name, query, SQLITE_OPEN_READWRITE | SQLITE_OPEN_NOMUTEX);

    err = sqlite3_bind_double(stmt.get(), 1, std::get<1-1>(gocator));
    err = sqlite3_bind_double(stmt.get(), 2, std::get<2-1>(gocator));
    err = sqlite3_bind_double(stmt.get(), 3, std::get<3-1>(gocator));
    err = sqlite3_bind_double(stmt.get(), 4, std::get<4-1>(gocator));

    tie(err, stmt) = db_step(move(stmt));

    return err == SQLITE_OK || err == SQLITE_DONE;
  }

  std::tuple<std::tuple<double,double,double,double>,int> fetch_scan_gocator(std::string db_name) 
  {
    auto query = R"(SELECT ZRes, ZOff, ZCard, Framerate FROM GOCATOR)"s;
    auto [stmt,db] = prepare_query(db_name, query);
    auto err = SQLITE_OK;

    tie(err, stmt) = db_step(move(stmt));

    std::tuple<std::tuple<double,double,double,double>,int> rtn;
    if (err == SQLITE_ROW)
    {

      rtn = {
          {sqlite3_column_double(stmt.get(), 0),
           sqlite3_column_double(stmt.get(), 1),
           sqlite3_column_double(stmt.get(), 2),
           sqlite3_column_double(stmt.get(), 3)},
          0};
    }
    else
    {
      rtn = {{1.0, 1.0, 1.0, 0.0}, -1};
    }

    return rtn;
  }

  bool store_scan_properties(std::tuple<date::utc_clock::time_point,date::utc_clock::time_point> props, std::string db_name) 
  {
    
    auto err = db_exec(db_name, R"(CREATE TABLE IF NOT EXISTS PROPERTIES (Width REAL NOT NULL, Length REAL NOT NULL, ScanBegin TEXT NOT NULL, ScanEnd TEXT NOT NULL, Org TEXT NOT NULL, ConveyorId INTEGER NOT NULL, BeltId INTEGER NOT NULL))");
    
    auto query = R"(INSERT INTO PROPERTIES (Width, Length, ScanBegin, ScanEnd, Org, ConveyorId, BeltId) VALUES (?,?,?,?,?,?,?))"s;

    auto [stmt,db] = prepare_query(db_name, query, SQLITE_OPEN_READWRITE | SQLITE_OPEN_NOMUTEX);

    auto ScanBegin = date::format("%FT%TZ",std::get<0>(props));
    auto ScanEnd = date::format("%FT%TZ",std::get<1>(props));
    auto [ConveyorId,errorc] = fetch_conveyor_id(); 
    auto [BeltId, errorb] = fetch_belt_id();
 

    err = sqlite3_bind_double(stmt.get(), 1, global_belt_parameters.Width);
    err = sqlite3_bind_double(stmt.get(), 2, global_belt_parameters.Length);
    err = sqlite3_bind_text(stmt.get(), 3, ScanBegin.c_str(),ScanBegin.size(),nullptr);
    err = sqlite3_bind_text(stmt.get(), 4, ScanEnd.c_str(),ScanEnd.size(),nullptr);
    err = sqlite3_bind_text(stmt.get(), 5, global_conveyor_parameters.Org.c_str(), global_conveyor_parameters.Org.size(), nullptr);
    err = sqlite3_bind_int64(stmt.get(), 6, ConveyorId);
    err = sqlite3_bind_int64(stmt.get(), 7, BeltId);


    tie(err, stmt) = db_step(move(stmt));

    return err == SQLITE_OK || err == SQLITE_DONE;
  }


  coro<int, z_type, 1> store_scan_coro(std::string db_name)
  {
    int err = SQLITE_ERROR;
    {
      std::string func_name = __func__;

      err = db_exec(db_name, R"(CREATE TABLE IF NOT EXISTS ZS (Z BLOB NOT NULL))"s);

      auto query = R"(INSERT INTO ZS (z) VALUES (?))"s;
      auto [stmt,db] = prepare_query(db_name, query, SQLITE_OPEN_READWRITE | SQLITE_OPEN_NOMUTEX);
      db_exec(db.get(), R"(PRAGMA synchronous=OFF)"s);

      while (true)
      {
        auto [z, terminate] = co_yield err;

        if (terminate)
          break;

        auto n = z.size() * sizeof(z_element);
        if (n > std::numeric_limits<int>::max())
        {
          terminate = true;
          std::throw_with_nested(std::runtime_error("z data size greater than sqlite limits"));
        }

        err = sqlite3_bind_blob(stmt.get(), 1, z.data(), (int)n, SQLITE_STATIC);
        
        if (err != SQLITE_OK)
        {
          terminate = true;
          spdlog::get("cads")->error("{}: sqlite3_bind_blob() error code:{}", func_name, err);
        }
      
        // Run once, retrying not effective, too slow and causes buffers to fill
        err = sqlite3_step(stmt.get());

        if (err != SQLITE_DONE)
        {
          terminate = true;
          spdlog::get("cads")->error("{}: sqlite3_step() error code:{}", func_name, err);
        }
        
        err = sqlite3_reset(stmt.get());

        if (err != SQLITE_OK)
        {
          terminate = true;
          spdlog::get("cads")->error("{}: sqlite3_reset() error code:{}", func_name, err);
        }

        if (terminate)
          break;
      }
    }

    if(err != SQLITE_OK) {
      std::filesystem::remove(db_name);
    }

    co_return err;
  }

  coro<std::tuple<int, z_type>> fetch_scan_coro(long first_index, long last_idx, std::string db_name, int size)
  {
    auto query = R"(SELECT rowid,z FROM ZS WHERE rowid >= ? AND rowid < ?)";
    auto [stmt,db] = prepare_query(db_name, query);

    for (long i = first_index; i < last_idx; i += size)
    {
      auto iend = i + size; 
      if(iend > last_idx) iend = last_idx;
      
      auto [p, s] = fetch_scan_coro_step(std::move(stmt), i, iend);
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
