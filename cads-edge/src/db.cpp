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

   [[maybe_unused]] std::tuple<sqlite3_stmt_t,sqlite3_t> prepare_query(sqlite3_t db, std::string query) {
    sqlite3_stmt *stmt_raw = nullptr;
    auto err = sqlite3_prepare_retry(db.get(), query.c_str(), (int)query.size(), &stmt_raw, NULL);

    if (err != SQLITE_OK)
    {
      auto errmsg = sqlite3_errstr(err);
      std::throw_with_nested(std::runtime_error(fmt::format("{} : sqlite3_prepare_retry {},{}",__func__,err, errmsg)));
    }

    sqlite3_stmt_t stmt(stmt_raw, &sqlite3_finalize);
    return {move(stmt),move(db)};
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
  void create_profile_db(std::string name = ""s)
  {
    auto db_name = name.empty() ? global_config["profile_db_name"].get<std::string>() : name;

    std::vector<std::string> tables{
        R"(PRAGMA journal_mode=WAL)"s,
        R"(DROP TABLE IF EXISTS PROFILE;)"s,
        R"(DROP TABLE IF EXISTS PARAMETERS;)"s,
        R"(CREATE TABLE IF NOT EXISTS PROFILE (revid INTEGER NOT NULL, idx INTEGER NOT NULL,y REAL NOT NULL, x_off REAL NOT NULL, z BLOB NOT NULL, PRIMARY KEY (revid,idx));)"s,
        R"(CREATE TABLE IF NOT EXISTS PARAMETERS (x_res REAL NOT NULL, z_res REAL NOT NULL, z_off REAL NOT NULL))"s
        };
    

    std::filesystem::remove(db_name);
    std::filesystem::remove(db_name + "-shm");
    std::filesystem::remove(db_name + "-wal");


    auto err = db_exec(db_name,tables);
    
    if(err != SQLITE_OK) {
      std::throw_with_nested(std::runtime_error("create_profile_db"));
    }

  }

  void create_program_state_db(std::string name = ""s)
  {
    using namespace date;
    using namespace std::chrono;

    auto db_name = name.empty() ? global_config["state_db_name"].get<std::string>() : name;
    
    std::vector<std::string> tables{
      R"(PRAGMA journal_mode=WAL)"s,
      R"(CREATE TABLE IF NOT EXISTS MOTIFS (date TEXT NOT NULL, motif BLOB NOT NULL))",
      R"(CREATE TABLE IF NOT EXISTS SCANS (
        scanned_utc TEXT NOT NULL UNIQUE
        ,db_name TEXT NOT NULL UNIQUE
        ,begin_index INTEGER NOT NULL
        ,cardinality INTEGER NOT NULL
        ,uploaded INTEGER NOT NULL
        ,status INTEGER NOT NULL
        ,conveyor_id INTEGER NOT NULL))",
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

  int store_profile_parameters(GocatorProperties params, std::string name)
  {
    auto query = R"(INSERT OR REPLACE INTO PARAMETERS (rowid,x_res,z_res,z_off) VALUES (1,?,?,?))"s;
    auto db_config_name = name.empty() ? global_config["profile_db_name"].get<std::string>() : name;
    auto [stmt,db] = prepare_query(db_config_name, query, SQLITE_OPEN_READWRITE | SQLITE_OPEN_NOMUTEX);

    auto err = sqlite3_bind_double(stmt.get(), 1, params.xResolution);
    err = sqlite3_bind_double(stmt.get(), 2, params.zResolution);
    err = sqlite3_bind_double(stmt.get(), 3, params.zOffset);


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

  std::tuple<cads::GocatorProperties, int> fetch_profile_parameters(std::string name)
  {

    auto query = R"(SELECT x_res,z_res,z_off FROM PARAMETERS WHERE ROWID = 1)"s;
    auto db_config_name = name.empty() ? global_config["profile_db_name"].get<std::string>() : name;
    auto [stmt,db] = prepare_query(db_config_name, query);
    auto err = SQLITE_OK;

    tie(err, stmt) = db_step(move(stmt));

    std::tuple<GocatorProperties, int> rtn;
    if (err == SQLITE_ROW)
    {

      rtn = {
          {sqlite3_column_double(stmt.get(), 0),
           sqlite3_column_double(stmt.get(), 1),
           sqlite3_column_double(stmt.get(), 2)},
          0};
    }
    else
    {
      rtn = {{1.0, 1.0, 1.0}, -1};
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

    long zero_seq = 0;
    
    while (true)
    {
      auto [data, terminate] = co_yield err;

      if (terminate)
        break;

      auto [rev, idx, p] = data;

      if(p.z.size() == 0) {
          
          if(zero_seq++ == 0) {
            spdlog::get("cads")->error(R"({{func = '{}', msg = '{}'}})", __func__,"No z samples. All sequential no z samples suppressed");
          }
          continue;
      }

      zero_seq = 0;

      if (p.y == std::numeric_limits<decltype(p.y)>::max())
        break;

      err = sqlite3_bind_int(stmt.get(), 1, rev);
      if (err != SQLITE_OK)
      {
         spdlog::get("cads")->error(R"({{func = '{}', fn = '{}', rtn = {}, msg = '', line = {}}})", __func__,"sqlite3_bind_int",err,__LINE__);
      }

      err = sqlite3_bind_int(stmt.get(), 2, idx);
      if (err != SQLITE_OK)
      {
         spdlog::get("cads")->error(R"({{func = '{}', fn = '{}', rtn = {}, msg = '', line = {}}})", __func__,"sqlite3_bind_int",err,__LINE__);
      }
      
      err = sqlite3_bind_double(stmt.get(), 3, p.y);
      if (err != SQLITE_OK)
      {
         spdlog::get("cads")->error(R"({{func = '{}', fn = '{}', rtn = {}, msg = '', line = {}}})", __func__,"sqlite3_bind_double",err,__LINE__);
      }

      err = sqlite3_bind_double(stmt.get(), 4, p.x_off);
      if (err != SQLITE_OK)
      {
         spdlog::get("cads")->error(R"({{func = '{}', fn = '{}', rtn = {}, msg = '', line = {}}})", __func__,"sqlite3_bind_double",err,__LINE__);
      }

      auto n = p.z.size() * sizeof(z_element);
      if (n > std::numeric_limits<int>::max())
      {
        std::throw_with_nested(std::runtime_error("z data size greater than sqlite limits"));
      }
      
      err = sqlite3_bind_blob(stmt.get(), 5, p.z.data(), (int)n, SQLITE_STATIC);
      
      if (err != SQLITE_OK)
      {
         spdlog::get("cads")->error(R"({{func = '{}', fn = '{}', rtn = {}, msg = ''}})", __func__,"sqlite3_bind_blob",err);
      }

      // Run once, retrying not effective, too slow and causes buffers to fill
      err = sqlite3_step(stmt.get());

      if(err != SQLITE_DONE) {
        spdlog::get("cads")->error(R"({{func = '{}', fn = '{}', rtn = {}, msg = ''}})", __func__,"sqlite3_step",err);
      }

      err = sqlite3_reset(stmt.get());

      if (err != SQLITE_OK)
      {
         spdlog::get("cads")->error(R"({{func = '{}', fn = '{}', rtn = {}, msg = ''}})", __func__,"sqlite3_reset",err);
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

        rtn.push_back({idx, {std::chrono::high_resolution_clock::now(),y, x_off, {z, z + len}}}); //FIXME
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

  std::tuple<date::utc_clock::time_point,std::vector<double>> fetch_last_motif(std::string name)
  {

    auto query = R"(SELECT date, motif FROM MOTIFS order by rowid desc limit 1)"s;
    auto db_config_name = name.empty() ? global_config["state_db_name"].get<std::string>() : name;
    auto [stmt,db] = prepare_query(db_config_name, query);
    
    std::tuple<date::utc_clock::time_point,std::vector<double>>  rtn;
    
    int err = SQLITE_ERROR;

    do{
      
      tie(err, stmt) = db_step(move(stmt));

      if (err == SQLITE_ROW)
      {
        
        double *z = (double *)sqlite3_column_blob(stmt.get(), 1); 
        int len = sqlite3_column_bytes(stmt.get(), 1) / sizeof(*z); 
        
        rtn = std::make_tuple(
          to_clk(std::string( (const char* )sqlite3_column_text(stmt.get(), 0), sqlite3_column_bytes(stmt.get(), 0))),
          std::vector<double>{z,z+len}
        );


      }
    }while(err == SQLITE_ROW);
    
    return rtn;
  }

  bool store_motif_state(std::tuple<date::utc_clock::time_point,std::vector<double>> row, std::string db_name)
  {
    using namespace std;
    auto query = R"(INSERT INTO MOTIFS (date,motif) VALUES(?,?))"s;
    auto db_config_name = db_name.empty() ? global_config["state_db_name"].get<std::string>() : db_name;
    auto [stmt,db] = prepare_query(db_config_name, query, SQLITE_OPEN_READWRITE | SQLITE_OPEN_NOMUTEX);
    
    auto date = to_str(get<0>(row));
    auto motif = get<1>(row);

    auto err = sqlite3_bind_text(stmt.get(), 1, date.c_str(), date.size(), nullptr);
    
    if (err != SQLITE_OK)
    {
      spdlog::get("cads")->error("{}: sqlite3_bind_text() error code:{}", __func__, err);
      return true;
    }

    err = sqlite3_bind_blob(stmt.get(), 2, motif.data(), (int)motif.size(), SQLITE_STATIC);
        
    if (err != SQLITE_OK)
    {
      spdlog::get("cads")->error("{}: sqlite3_bind_blob() error code:{}", __func__, err);
      return true;
    }
      
    tie(err, stmt) = db_step(move(stmt));

    if (err != SQLITE_DONE)
    {
      spdlog::get("cads")->error("{}: db_step() error code:{}", __func__, err);
      return true;
    }
        
    return false;
  }


  std::deque<state::scan> fetch_scan_state(std::string name)
  {

    auto query = R"(SELECT 
      scanned_utc
      ,db_name
      ,begin_index
      ,cardinality
      ,uploaded
      ,status 
      ,conveyor_id 
      FROM Scans)"s;
    auto db_config_name = name.empty() ? global_config["state_db_name"].get<std::string>() : name;
    auto [stmt,db] = prepare_query(db_config_name, query);
    
    std::deque<state::scan>  rtn;
    
    int err = SQLITE_ERROR;

    do{
      
      tie(err, stmt) = db_step(move(stmt));

      if (err == SQLITE_ROW)
      {
        
        state::scan tmp = {
          to_clk(std::string( (const char* )sqlite3_column_text(stmt.get(), 0), sqlite3_column_bytes(stmt.get(), 0))),
          std::string( (const char* )sqlite3_column_text(stmt.get(), 1), sqlite3_column_bytes(stmt.get(), 1)),
          int64_t(sqlite3_column_int64(stmt.get(), 2)),
          int64_t(sqlite3_column_int64(stmt.get(), 3)),
          int64_t(sqlite3_column_int64(stmt.get(), 4)),
          int64_t(sqlite3_column_int64(stmt.get(), 5)),
          int64_t(sqlite3_column_int64(stmt.get(), 6))
        };

        rtn.push_back(tmp); 
      }
    }while(err == SQLITE_ROW);
    
    return rtn;
  }

  bool update_scan_state(state::scan scan, std::string db_name)
  {

    auto query = R"(update scans set (
      scanned_utc
      ,db_name
      ,begin_index
      ,cardinality
      ,uploaded
      ,status
      ,conveyor_id 
      ) = (?,?,?,?,?,?,?) where db_name=?;)"s;
    auto db_config_name = db_name.empty() ? global_config["state_db_name"].get<std::string>() : db_name;
    auto [stmt,db] = prepare_query(db_config_name, query, SQLITE_OPEN_READWRITE | SQLITE_OPEN_NOMUTEX);
    
    auto scanned_utc = to_str(scan.scanned_utc);

    auto err = sqlite3_bind_text(stmt.get(), 1, scanned_utc.c_str(), scanned_utc.size(),nullptr);
    err = sqlite3_bind_text(stmt.get(), 2, scan.db_name.c_str(), scan.db_name.size(),nullptr);
    err = sqlite3_bind_int64(stmt.get(), 3, scan.begin_index);
    err = sqlite3_bind_int64(stmt.get(), 4, scan.cardinality);
    err = sqlite3_bind_int64(stmt.get(), 5, scan.uploaded);
    err = sqlite3_bind_int64(stmt.get(), 6, scan.status); 
    err = sqlite3_bind_int64(stmt.get(), 7, scan.conveyor_id); 
    err = sqlite3_bind_text(stmt.get(), 8, scan.db_name.c_str(), scan.db_name.size(),nullptr);

    tie(err, stmt) = db_step(move(stmt));

    return err == SQLITE_OK || err == SQLITE_DONE;
  }


  bool store_scan_state(state::scan scan, std::string db_name)
  {
    auto query = R"(INSERT INTO SCANS (
      scanned_utc
      ,db_name
      ,begin_index
      ,cardinality
      ,uploaded
      ,status
      ,conveyor_id
      ) VALUES(?,?,?,?,?,?,?))"s;
    auto db_config_name = db_name.empty() ? global_config["state_db_name"].get<std::string>() : db_name;
    auto [stmt,db] = prepare_query(db_config_name, query, SQLITE_OPEN_READWRITE | SQLITE_OPEN_NOMUTEX);
    
    auto scanned_utc = to_str(scan.scanned_utc);

    auto err = sqlite3_bind_text(stmt.get(), 1, scanned_utc.c_str(), scanned_utc.size(),nullptr);
    err = sqlite3_bind_text(stmt.get(), 2, scan.db_name.c_str(), scan.db_name.size(),nullptr);
    err = sqlite3_bind_int64(stmt.get(), 3, scan.begin_index);
    err = sqlite3_bind_int64(stmt.get(), 4, scan.cardinality);
    err = sqlite3_bind_int64(stmt.get(), 5, scan.uploaded);
    err = sqlite3_bind_int64(stmt.get(), 6, scan.status); 
    err = sqlite3_bind_int64(stmt.get(), 7, scan.conveyor_id); 
 
    tie(err, stmt) = db_step(move(stmt));

    return err == SQLITE_OK || err == SQLITE_DONE;
  }

  bool delete_scan_state(cads::state::scan scan, std::string db_name)
  {
    auto query = R"(DELETE FROM Scans WHERE db_name = ?)"s;
    auto db_config_name = db_name.empty() ? global_config["state_db_name"].get<std::string>() : db_name;
    auto [stmt,db] = prepare_query(db_config_name, query, SQLITE_OPEN_READWRITE | SQLITE_OPEN_NOMUTEX);
    
    auto err = sqlite3_bind_text(stmt.get(), 1, scan.db_name.c_str(), scan.db_name.size(),nullptr);


    tie(err, stmt) = db_step(move(stmt));

    return err == SQLITE_OK || err == SQLITE_DONE;
  }


  // scan db

  bool store_scan_gocator(cads::GocatorProperties gocator, std::string db_name) 
  {
    
    auto err = db_exec(db_name, R"(CREATE TABLE IF NOT EXISTS GOCATOR(XRes REAL NOT NULL, ZRes REAL NOT NULL, ZOff REAL NOT NULL))"s);
    
    auto query = R"(INSERT INTO GOCATOR (XRes, ZRes, ZOff) VALUES (?,?,?))"s;

    auto [stmt,db] = prepare_query(db_name, query, SQLITE_OPEN_READWRITE | SQLITE_OPEN_NOMUTEX);

    err = sqlite3_bind_double(stmt.get(), 1, gocator.xResolution);
    err = sqlite3_bind_double(stmt.get(), 2, gocator.zResolution);
    err = sqlite3_bind_double(stmt.get(), 3, gocator.zOffset);

    tie(err, stmt) = db_step(move(stmt));

    return err == SQLITE_OK || err == SQLITE_DONE;
  }

  std::tuple<cads::GocatorProperties,int> fetch_scan_gocator(std::string db_name) 
  {
    auto query = R"(SELECT XRes, ZRes, ZOff FROM GOCATOR)"s;
    auto [stmt,db] = prepare_query(db_name, query);
    auto err = SQLITE_OK;

    tie(err, stmt) = db_step(move(stmt));

    std::tuple<cads::GocatorProperties,int> rtn;
    if (err == SQLITE_ROW)
    {

      rtn = {
          {sqlite3_column_double(stmt.get(), 0),
           sqlite3_column_double(stmt.get(), 1),
           sqlite3_column_double(stmt.get(), 2)},
          0};
    }
    else
    {
      rtn = {cads::GocatorProperties{}, -1};
    }

    return rtn;
  }

  bool store_scan_conveyor(cads::Conveyor conveyor, std::string db_name) 
  {
    auto err = db_exec(db_name, R"(CREATE TABLE IF NOT EXISTS Conveyor (
      Id INTEGER PRIMARY KEY
      ,Org TEXT NOT NULL 
      ,Site TEXT NOT NULL 
      ,Name TEXT NOT NULL 
      ,Timezone TEXT NOT NULL 
      ,PulleyCircumference REAL NOT NULL 
      ,TypicalSpeed REAL NOT NULL 
      ,Belt INTEGER NOT NULL
      ,Length REAL NOT NULL 
      ,WidthN INTEGER NOT NULL
      ))");
    
    auto query = R"(INSERT INTO Conveyor (
      Id
      ,Org
      ,Site
      ,Name 
      ,Timezone
      ,PulleyCircumference
      ,TypicalSpeed
      ,Belt
      ,Length
      ,WidthN
    ) VALUES (?,?,?,?,?,?,?,?,?,?))"s;
  
    auto [stmt,db] = prepare_query(db_name, query, SQLITE_OPEN_READWRITE | SQLITE_OPEN_NOMUTEX);

    err = sqlite3_bind_int64(stmt.get(), 1, conveyor.Id);
    err = sqlite3_bind_text(stmt.get(), 2, conveyor.Org.c_str(),conveyor.Org.size(),nullptr);
    err = sqlite3_bind_text(stmt.get(), 3, conveyor.Site.c_str(),conveyor.Site.size(),nullptr);
    err = sqlite3_bind_text(stmt.get(), 4, conveyor.Name.c_str(), conveyor.Name.size(), nullptr);
    err = sqlite3_bind_text(stmt.get(), 5, conveyor.Timezone.c_str(), conveyor.Timezone.size(), nullptr);
    err = sqlite3_bind_double(stmt.get(), 6, conveyor.PulleyCircumference);
    err = sqlite3_bind_double(stmt.get(), 7, conveyor.TypicalSpeed);
    err = sqlite3_bind_int64(stmt.get(), 8, conveyor.Belt);
    err = sqlite3_bind_double(stmt.get(), 9, conveyor.Length);
    err = sqlite3_bind_int64(stmt.get(), 10, conveyor.WidthN);

    tie(err, stmt) = db_step(move(stmt));

    return err == SQLITE_OK || err == SQLITE_DONE;
  }

  std::tuple<cads::Conveyor,int> fetch_scan_conveyor(std::string db_name) 
  {
    auto query = R"(SELECT
      Id
      ,Org
      ,Site
      ,Name 
      ,Timezone
      ,PulleyCircumference
      ,TypicalSpeed
      ,Belt
      ,Length
      ,WidthN
     FROM Conveyor)"s;

    auto [stmt,db] = prepare_query(db_name, query);
    auto err = SQLITE_OK;

    tie(err, stmt) = db_step(move(stmt));

    std::tuple<cads::Conveyor,int> rtn;
    if (err == SQLITE_ROW)
    {

      rtn = {
          {int64_t(sqlite3_column_int64(stmt.get(), 0)),  
          std::string( (const char* )sqlite3_column_text(stmt.get(), 1), sqlite3_column_bytes(stmt.get(), 1)),
          std::string( (const char* )sqlite3_column_text(stmt.get(), 2), sqlite3_column_bytes(stmt.get(), 2)),
          std::string( (const char* )sqlite3_column_text(stmt.get(), 3), sqlite3_column_bytes(stmt.get(), 3)),
          std::string( (const char* )sqlite3_column_text(stmt.get(), 4), sqlite3_column_bytes(stmt.get(), 4)),
          double(sqlite3_column_double(stmt.get(), 5)),
          double(sqlite3_column_double(stmt.get(), 6)),
          int64_t(sqlite3_column_int64(stmt.get(), 7)),
          double(sqlite3_column_double(stmt.get(), 8)),
          int64_t(sqlite3_column_int64(stmt.get(), 9))},
          0};
    }
    else
    {
      rtn = {cads::Conveyor{}, -1};
    }

    return rtn;
  }

  bool transfer_profiles(std::string from_db_name, std::string to_db_name, int64_t first_index, int64_t last_index)
  {
    auto err = db_exec(to_db_name, R"(CREATE TABLE IF NOT EXISTS ZS (Z BLOB NOT NULL))"s);
    
    if(err != SQLITE_OK) return false;
    
    auto from_query = R"(SELECT rowid,z FROM ZS WHERE rowid >= ? AND rowid < ?)";
    auto to_query = R"(INSERT INTO ZS (z) VALUES (?))"s;
    auto [from_stmt,from_db] = prepare_query(from_db_name, from_query, SQLITE_OPEN_READWRITE | SQLITE_OPEN_NOMUTEX);
    auto [to_stmt,to_db] = prepare_query(to_db_name, to_query, SQLITE_OPEN_READWRITE | SQLITE_OPEN_NOMUTEX);

    err = sqlite3_bind_int64(from_stmt.get(), 1, first_index); 

    if(err != SQLITE_OK) return false;
      
    err = sqlite3_bind_int64(from_stmt.get(), 2, last_index);

    if(err != SQLITE_OK) return false;
    
    do{
      
      tie(err, from_stmt) = db_step(move(from_stmt));

      if (err == SQLITE_ROW)
      {
          z_element *z = (z_element *)sqlite3_column_blob(from_stmt.get(), 1); // Freed on next call to sqlite3_step
          int len = sqlite3_column_bytes(from_stmt.get(), 1) / sizeof(*z);

          auto bind_err = sqlite3_bind_blob(to_stmt.get(), 2, z, len, SQLITE_STATIC);
          tie(bind_err, to_stmt) = db_step(move(to_stmt));
          if (bind_err != SQLITE_DONE) {
            return false;
          }
      }
    }while(err == SQLITE_ROW);

    return err == SQLITE_DONE;
    
  }

  bool create_scan_db(std::string db_name) {
    auto err = db_exec(db_name, R"(CREATE TABLE IF NOT EXISTS ZS (Z BLOB NOT NULL))"s);
    return err == SQLITE_OK;
  }

  coro<int, z_type, 1> store_scan_coro(std::string db_name)
  {
    spdlog::get("cads")->debug(R"({{func = '{}', msg = '{}' args='{}'}})", __func__,"Entering",db_name);
    
    int err = SQLITE_ERROR;
    {
      err = db_exec(db_name, R"(CREATE TABLE IF NOT EXISTS ZS (Z BLOB NOT NULL))"s);

      auto query = R"(INSERT INTO ZS (z) VALUES (?))"s;
      auto [stmt,db] = prepare_query(db_name, query, SQLITE_OPEN_READWRITE | SQLITE_OPEN_NOMUTEX);
      db_exec(db.get(), R"(PRAGMA synchronous=OFF)"s);

      long zero_seq = 0;

      while (true)
      {
        auto [z, terminate] = co_yield err;

        if (terminate)
          break;

        if(z.size() == 0 ) {
          
          if(zero_seq++ == 0) {
            spdlog::get("cads")->error(R"({{func = '{}', msg = '{}'}})", __func__,"No z samples. All sequential no z samples suppressed");
          }
          continue;
        }

        zero_seq = 0;
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
          spdlog::get("cads")->error(R"({{func = '{}', fn = '{}', rtn = {}, msg = ''}})", __func__,"sqlite3_bind_blob",err);
        }
      
        // Run once, retrying not effective, too slow and causes buffers to fill
        err = sqlite3_step(stmt.get());

        if (err != SQLITE_DONE)
        {
          terminate = true;
          spdlog::get("cads")->error(R"({{func = '{}', fn = '{}', rtn = {}, msg = '{}'}})", __func__,"sqlite3_step",err,db_name);
        }
       
        err = sqlite3_reset(stmt.get());

        if (err != SQLITE_OK)
        {
          terminate = true;
          spdlog::get("cads")->error(R"({{func = '{}', fn = '{}', rtn = {}, msg = '{}'}})", __func__,"sqlite3_reset",err,db_name);
        }

        if (terminate)
          break;
      }
    }

    if(err != SQLITE_OK) {
      std::filesystem::remove(db_name);
    }

    spdlog::get("cads")->debug(R"({{func = '{}', rtn = {}, msg = '{}'}})", __func__,err,db_name);
    co_return err;
  }

  coro<std::tuple<int, z_type>> fetch_scan_coro(long first_index, long last_idx, std::string db_name, int size)
  {
    spdlog::get("cads")->debug(R"({{func = '{}', msg = '{}' args='{}'}})", __func__,"Entering",db_name);
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

    spdlog::get("cads")->debug(R"({{func = '{}', msg = '{}'}})", __func__,"Exiting");
  }

  std::deque<std::tuple<int, cads::z_type>> fetch_scan(long first_index, long last_idx, std::string db_name, int size)
  {
    auto query = R"(SELECT rowid,z FROM ZS WHERE rowid >= ? AND rowid < ?)";
    auto [stmt,db] = prepare_query(db_name, query);

    auto iend = first_index + size; 
    if(iend > last_idx) iend = last_idx;
    
    auto [p, s] = fetch_scan_coro_step(std::move(stmt), first_index, iend);

    return p;
      
  }


}
