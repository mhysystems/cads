#include <vector>
#include <string>
#include <iostream>
#include <filesystem>
#include <memory>
#include <sstream>
#include <chrono>

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wuseless-cast"
#pragma GCC diagnostic ignored "-Wshadow"
#pragma GCC diagnostic ignored "-Wunused-parameter"
#pragma GCC diagnostic ignored "-Wconversion"
#pragma GCC diagnostic ignored "-Wnon-virtual-dtor"
#pragma GCC diagnostic ignored "-Wreorder"
#pragma GCC diagnostic ignored "-Wuseless-cast"
#pragma GCC diagnostic ignored "-Wdangling-reference"

#include <fmt/core.h>
#include <fmt/chrono.h>
#include <spdlog/spdlog.h>

#pragma GCC diagnostic pop

#include <db.h>
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
 int sqlite3_bind(sqlite3_stmt *s, int index, int64_t value)
  {
    return sqlite3_bind_int64(s, index, value);
  }

  int sqlite3_bind(sqlite3_stmt *s, int index, double value)
  {
    return sqlite3_bind_double(s, index, value);
  }

  int sqlite3_bind(sqlite3_stmt *s, int index, std::string value)
  {
    return sqlite3_bind_text(s, index, value.c_str(), value.size(), nullptr);
  }

  template<class ...T, int ...N> auto zippy_bind(sqlite3_stmt *s, std::tuple<T...> t,std::integer_sequence<int,N...>) {
    
    return (0 || ... || sqlite3_bind(s,N+1,std::get<N>(t)));
  }

  template<class ...T> int bind(sqlite3_stmt *s,std::tuple<T...> t)
  {
    return zippy_bind(s,t,std::make_integer_sequence<int,sizeof...(T)>{}); 
  }

  template<class T> std::tuple<T,int> sqlite3_column(sqlite3_stmt *s,int index);

  template<> std::tuple<int64_t,int> sqlite3_column<int64_t>(sqlite3_stmt *s, int index)
  {
    auto value = sqlite3_column_int64(s,index);
    return {value,0};
  }

  template<> std::tuple<double,int> sqlite3_column<double>(sqlite3_stmt *s, int index)
  {
    auto value = sqlite3_column_double(s,index);
    return {value,0};
  }

  template<> std::tuple<std::string,int> sqlite3_column<std::string>(sqlite3_stmt *s, int index)
  {
    std::string value( (const char* )sqlite3_column_text(s, index), sqlite3_column_bytes(s, index));
    return {value,0};
  }

  template<class ...T, int ...N> auto zippy_column(sqlite3_stmt *s,std::integer_sequence<int,N...>) {
    return std::make_tuple(sqlite3_column<typename std::conditional<std::is_enum_v<T>,int64_t,T>::type>(s,N)...);
  }

  template<class ...T> struct column
  {
    static std::expected<std::tuple<T...>,int> get(sqlite3_stmt *s)
    {

      auto r = zippy_column<T...>(s,std::make_integer_sequence<int,sizeof...(T)>{});
      auto error = std::apply([=](auto&&...e){return (0 || ... || std::get<1>(e));},r);

      if(error) {
        return std::unexpected(error);
      }else{
        auto row = std::apply([=](auto&&...e){return std::make_tuple(std::get<0>(e)...);},r);
        return row;
      }
    }
  };

  template<class T> std::string sqlite_type_names();

  template<> std::string sqlite_type_names<double>()
  {
    using namespace std::string_literals;
    return "REAL";
  }

  template<> std::string sqlite_type_names<std::string>()
  {
    using namespace std::string_literals;
    return "TEXT";
  }

  template<> std::string sqlite_type_names<int64_t>()
  {
    using namespace std::string_literals;
    return "INTEGER";
  }

  template<class ...T> std::array<std::string,sizeof...(T)> creation_types(std::tuple<T...> )
  {
    return std::array{sqlite_type_names<T>()...};
  }

  template<int ...N> std::array<std::string,sizeof...(N)> repeat_n(std::string s, std::integer_sequence<int,N...>)
  {
    using namespace std::string_literals;
    auto f = [=](int){return s;};
    return std::array{f(N)...};
  }

  template<int N> std::array<std::string,N> repeat(std::string s)
  {
    return repeat_n(s,std::make_integer_sequence<int,N>{});
  }
  
  template<class ...T> auto create_insert(std::tuple<std::string,std::tuple<std::tuple<std::string,T>...>> table, std::string db_filename)
  {
    using namespace std::literals;
    using table_types = std::tuple<std::tuple<std::string,T>...>;
    
    auto type_names = std::array{sqlite_type_names<T>()...};
    auto field_names = std::apply([=](auto&&... e) {return std::array{std::get<0>(e)...};},std::get<1>(table));
    auto field_values = std::apply([=](auto&&... e) {return std::make_tuple(std::get<1>(e)...);},std::get<1>(table));
    constexpr int field_no = std::tuple_size<table_types>::value;

    auto not_null = ::repeat<field_no>("NOT NULL"s);
    auto question_mark = ::repeat<field_no>("?"s);

    auto create = std::views::zip_transform([](std::string name, std::string type, std::string suffix){return name + ' ' + type + ' ' + suffix;},field_names,type_names,not_null);

    auto create_query = fmt::format("CREATE TABLE IF NOT EXISTS {} ({})",std::get<0>(table),fmt::join(create,","));  
    auto err = db_exec(db_filename, create_query);
    
    if(err) return err;

    auto insert_query = fmt::format("INSERT INTO {} ({}) VALUES({})",std::get<0>(table),fmt::join(field_names,","),fmt::join(question_mark,","));  

    auto [stmt,db] = prepare_query(db_filename, insert_query, SQLITE_OPEN_READWRITE | SQLITE_OPEN_NOMUTEX);

    err = bind(stmt.get(),field_values);

    if(err) return err;
    
    err = sqlite3_step(stmt.get());

    return err ;

  }

  template<class ...T> std::expected<std::tuple<T...>,int>  select_single (std::tuple<std::string,std::tuple<std::tuple<std::string,T>...>> table, std::string db_filename)
  {
    using namespace std::literals;
    
    auto type_names = std::array{sqlite_type_names<T>()...};
    auto field_names = std::apply([=](auto&&... e) {return std::array{std::get<0>(e)...};},std::get<1>(table));

    auto select_query = fmt::format("SELECT {} FROM {} LIMIT 1",fmt::join(field_names,","),std::get<0>(table));  

    auto [stmt,db] = prepare_query(db_filename, select_query, SQLITE_OPEN_READONLY | SQLITE_OPEN_NOMUTEX);

    auto err = sqlite3_step(stmt.get());
    if (err != SQLITE_ROW) return  std::unexpected(err);

    return column<T...>::get(stmt.get());

  }

}

namespace cads
{
  void create_profile_db(std::string name = ""s)
  {
    auto db_name = name.empty() ? database_names.profile_db_name : name;

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

    auto db_name = name.empty() ? database_names.state_db_name : name;
    
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
        ,site TEXT NOT NULL
        ,conveyor TEXT NOT NULL
        ,remote_reg INTEGER NOT NULL
        ))",
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

    auto db_name = name.empty() ? database_names.transient_db_name : name;
    
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
    auto db_config_name = name.empty() ? database_names.profile_db_name : name;
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
    auto db_config_name = name.empty() ? database_names.transient_db_name : name;
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
    auto db_config_name = name.empty() ? database_names.transient_db_name : name;
    auto [stmt,db] = prepare_query(db_config_name, query, SQLITE_OPEN_READWRITE | SQLITE_OPEN_NOMUTEX);
    
    sqlite3_exec(db.get(), R"(PRAGMA synchronous=OFF)"s.c_str(), nullptr, nullptr, nullptr);
    
    sqlite3_bind_text(stmt.get(), 1, id.c_str(), id.size(),nullptr);
    auto n = z.size() * sizeof(z_element);
    sqlite3_bind_blob(stmt.get(), 2, z.data(), (int)n, SQLITE_STATIC);

    sqlite3_step(stmt.get());
  }

  std::tuple<cads::GocatorProperties, int> fetch_profile_parameters(std::string name)
  {

    auto query = R"(SELECT XRes,ZRes,ZOff FROM GOCATOR WHERE ROWID = 1)"s;
    auto db_config_name = name.empty() ? database_names.profile_db_name : name;
    auto [stmt,db] = prepare_query(db_config_name, query);
    auto err = SQLITE_OK;

    tie(err, stmt) = db_step(move(stmt));

    std::tuple<GocatorProperties, int> rtn;
    if (err == SQLITE_ROW)
    {

      rtn = {
          {sqlite3_column_double(stmt.get(), 0),
           sqlite3_column_double(stmt.get(), 1),
           sqlite3_column_double(stmt.get(), 2),
           -1000.0,
           2000.0,
           -765.0,
           1500.0},
          0};
    }
    else
    {
      rtn = {cads::GocatorProperties{}, -1};
    }

    return rtn;
  }


  std::tuple<double, double, double, double, int> fetch_belt_dimensions(int revid, int idx, std::string name)
  {

    auto query = R"(SELECT MIN(Y),MAX(Y),COUNT(Y),LENGTH(Z) FROM PROFILE WHERE REVID = ? AND IDX < ? LIMIT 1)"s;
    auto db_config_name = name.empty() ? database_names.profile_db_name : name;
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
    auto db_config_name = name.empty() ? database_names.profile_db_name : name;
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
    auto db_config_name = name.empty() ? database_names.profile_db_name : name;
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
         spdlog::get("cads")->error(R"({{func = '{}', fn = '{}', rtn = {}, msg = '{}'}})", __func__,"sqlite3_bind_blob",err,db_config_name);
      }

      // Run once, retrying not effective, too slow and causes buffers to fill
      err = sqlite3_step(stmt.get());

      if(err != SQLITE_DONE) {
        spdlog::get("cads")->error(R"({{func = '{}', fn = '{}', rtn = {}, msg = '{}'}})", __func__,"sqlite3_step",err,db_config_name);
      }

      err = sqlite3_reset(stmt.get());

      if (err != SQLITE_OK)
      {
         spdlog::get("cads")->error(R"({{func = '{}', fn = '{}', rtn = {}, msg = '{}'}})", __func__,"sqlite3_reset",err,db_config_name);
      }
    }

    spdlog::get("db")->info("store_profile_coro finished");
    co_return err;
  }

  std::tuple<std::deque<std::tuple<int, profile>>, sqlite3_stmt_t> fetch_scan_coro_step(sqlite3_stmt_t stmt, int rowid_begin, int rowid_end)
  {

    std::deque<std::tuple<int, profile>> rtn;
    auto err = sqlite3_bind_int(stmt.get(), 1, rowid_begin);
    err = sqlite3_bind_int(stmt.get(), 2, rowid_end);

    while (true)
    {
      // Run once, retrying not effective, too slow and causes buffers to fill
      err = sqlite3_step(stmt.get());

      if (err == SQLITE_ROW)
      {

        auto rowid = sqlite3_column_int(stmt.get(), 0);
        auto y = sqlite3_column_double(stmt.get(), 1);
        auto x_off = sqlite3_column_double(stmt.get(), 2);

        z_element *z = (z_element *)sqlite3_column_blob(stmt.get(), 3); // Freed on next call to sqlite3_step
        int len = sqlite3_column_bytes(stmt.get(), 3) / sizeof(*z);

        rtn.push_back({rowid, profile{decltype(profile::time){},y,x_off,{z, z + len}}});
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
    //auto query = fmt::format(R"(SELECT idx,y,x_off,z FROM PROFILE WHERE REVID = {} AND IDX >= ? AND IDX < ?)", revid);
    auto query = fmt::format(R"(SELECT rowid - 1,y,x,z FROM Profiles WHERE rowid >= ? + 1 AND rowid < ? + 1)", revid);
    auto db_config_name = name.empty() ? database_names.profile_db_name : name;
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

  std::tuple<date::utc_clock::time_point,std::vector<double>> fetch_last_motif(std::string name)
  {

    auto query = R"(SELECT date, motif FROM MOTIFS order by rowid desc limit 1)"s;
    auto db_config_name = name.empty() ? database_names.state_db_name : name;
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
    auto db_config_name = db_name.empty() ? database_names.state_db_name : db_name;
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
      ,site
      ,conveyor
      ,remote_reg 
      FROM Scans)"s;
    auto db_config_name = name.empty() ? database_names.state_db_name : name;
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
          std::string( (const char* )sqlite3_column_text(stmt.get(), 6), sqlite3_column_bytes(stmt.get(), 6)),
          std::string( (const char* )sqlite3_column_text(stmt.get(), 7), sqlite3_column_bytes(stmt.get(), 7)),
          int64_t(sqlite3_column_int64(stmt.get(), 8))
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
      ,site
      ,conveyor
      ,remote_reg 
      ) = (?,?,?,?,?,?,?,?,?) where db_name=?;)"s;
    auto db_config_name = db_name.empty() ? database_names.state_db_name : db_name;
    auto [stmt,db] = prepare_query(db_config_name, query, SQLITE_OPEN_READWRITE | SQLITE_OPEN_NOMUTEX);
    
    auto scanned_utc = to_str(scan.scanned_utc);

    auto err = sqlite3_bind_text(stmt.get(), 1, scanned_utc.c_str(), scanned_utc.size(),nullptr);
    err = sqlite3_bind_text(stmt.get(), 2, scan.db_name.c_str(), scan.db_name.size(),nullptr);
    err = sqlite3_bind_int64(stmt.get(), 3, scan.begin_index);
    err = sqlite3_bind_int64(stmt.get(), 4, scan.cardinality);
    err = sqlite3_bind_int64(stmt.get(), 5, scan.uploaded);
    err = sqlite3_bind_int64(stmt.get(), 6, scan.status); 
    err = sqlite3_bind_text(stmt.get(), 7, scan.site.c_str(), scan.site.size(),nullptr);
    err = sqlite3_bind_text(stmt.get(), 8, scan.conveyor.c_str(), scan.conveyor.size(),nullptr);
    err = sqlite3_bind_int64(stmt.get(), 9, scan.remote_reg);
    err = sqlite3_bind_text(stmt.get(), 10, scan.db_name.c_str(), scan.db_name.size(),nullptr); 

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
      ,site
      ,conveyor
      ,remote_reg
      ) VALUES(?,?,?,?,?,?,?,?,?))"s;
    auto db_config_name = db_name.empty() ? database_names.state_db_name : db_name;
    auto [stmt,db] = prepare_query(db_config_name, query, SQLITE_OPEN_READWRITE | SQLITE_OPEN_NOMUTEX);
    
    auto scanned_utc = to_str(scan.scanned_utc);

    auto err = sqlite3_bind_text(stmt.get(), 1, scanned_utc.c_str(), scanned_utc.size(),nullptr);
    err = sqlite3_bind_text(stmt.get(), 2, scan.db_name.c_str(), scan.db_name.size(),nullptr);
    err = sqlite3_bind_int64(stmt.get(), 3, scan.begin_index);
    err = sqlite3_bind_int64(stmt.get(), 4, scan.cardinality);
    err = sqlite3_bind_int64(stmt.get(), 5, scan.uploaded);
    err = sqlite3_bind_int64(stmt.get(), 6, scan.status); 
    err = sqlite3_bind_text(stmt.get(), 7, scan.site.c_str(), scan.site.size(),nullptr);
    err = sqlite3_bind_text(stmt.get(), 8, scan.conveyor.c_str(), scan.conveyor.size(),nullptr);
    err = sqlite3_bind_int64(stmt.get(), 9, scan.remote_reg); 
 
    tie(err, stmt) = db_step(move(stmt));

    return err == SQLITE_OK || err == SQLITE_DONE;
  }

  bool delete_scan_state(cads::state::scan scan, std::string db_name)
  {
    auto query = R"(DELETE FROM Scans WHERE db_name = ?)"s;
    auto db_config_name = db_name.empty() ? database_names.state_db_name : db_name;
    auto [stmt,db] = prepare_query(db_config_name, query, SQLITE_OPEN_READWRITE | SQLITE_OPEN_NOMUTEX);
    
    auto err = sqlite3_bind_text(stmt.get(), 1, scan.db_name.c_str(), scan.db_name.size(),nullptr);


    tie(err, stmt) = db_step(move(stmt));

    return err == SQLITE_OK || err == SQLITE_DONE;
  }


  // scan db

  bool store_scan_gocator(cads::GocatorProperties gocator, std::string db_name) 
  {
    return (bool)create_insert(gocator.decompose(),db_name);
  }

  std::expected<cads::GocatorProperties,int> fetch_scan_gocator(std::string db_name) 
  {
    auto row = select_single(cads::GocatorProperties().decompose(),db_name);
    if(!row) {
      auto errmsg = sqlite3_errstr(row.error());
      spdlog::get("cads")->error(R"({{func = '{}', msg = '{}'}})", __func__,errmsg);
      return std::unexpected(row.error());
    }

    return std::apply([](auto&&... e){return cads::GocatorProperties{e...};}, *row);
  }


  bool store_scan_conveyor(cads::Conveyor conveyor, std::string db_name) 
  {
    auto err = db_exec(db_name, R"(CREATE TABLE IF NOT EXISTS Conveyor (
      Site TEXT NOT NULL 
      ,Name TEXT NOT NULL 
      ,Timezone TEXT NOT NULL 
      ,PulleyCircumference REAL NOT NULL 
      ,TypicalSpeed REAL NOT NULL 
      ))");
    
    auto query = R"(INSERT INTO Conveyor (
      Site
      ,Name 
      ,Timezone
      ,PulleyCircumference
      ,TypicalSpeed
    ) VALUES (?,?,?,?,?))"s;
  
    auto [stmt,db] = prepare_query(db_name, query, SQLITE_OPEN_READWRITE | SQLITE_OPEN_NOMUTEX);

    err = sqlite3_bind_text(stmt.get(), 1, conveyor.Site.c_str(),conveyor.Site.size(),nullptr);
    err = sqlite3_bind_text(stmt.get(), 2, conveyor.Name.c_str(), conveyor.Name.size(), nullptr);
    err = sqlite3_bind_text(stmt.get(), 3, conveyor.Timezone.c_str(), conveyor.Timezone.size(), nullptr);
    err = sqlite3_bind_double(stmt.get(), 4, conveyor.PulleyCircumference);
    err = sqlite3_bind_double(stmt.get(), 5, conveyor.TypicalSpeed);

    tie(err, stmt) = db_step(move(stmt));

    return err == SQLITE_OK || err == SQLITE_DONE;
  }

  std::tuple<cads::Conveyor,int> fetch_scan_conveyor(std::string db_name) 
  {
    auto query = R"(SELECT
      Site
      ,Name 
      ,Timezone
      ,PulleyCircumference
      ,TypicalSpeed
     FROM Conveyor)"s;

    auto [stmt,db] = prepare_query(db_name, query);
    auto err = SQLITE_OK;

    tie(err, stmt) = db_step(move(stmt));

    std::tuple<cads::Conveyor,int> rtn;
    if (err == SQLITE_ROW)
    {

      rtn = {
          {std::string( (const char* )sqlite3_column_text(stmt.get(), 0), sqlite3_column_bytes(stmt.get(), 0)),
          std::string( (const char* )sqlite3_column_text(stmt.get(), 1), sqlite3_column_bytes(stmt.get(), 1)),
          std::string( (const char* )sqlite3_column_text(stmt.get(), 2), sqlite3_column_bytes(stmt.get(), 2)),
          double(sqlite3_column_double(stmt.get(), 3)),
          double(sqlite3_column_double(stmt.get(), 4))},
          0};
    }
    else
    {
      rtn = {cads::Conveyor{}, -1};
    }

    return rtn;
  }


  bool store_scan_belt(cads::Belt belt, std::string db_name) 
  {
    auto err = db_exec(db_name, R"(CREATE TABLE IF NOT EXISTS Belt (
      Serial TEXT NOT NULL 
      ,PulleyCover REAL NOT NULL 
      ,CordDiameter REAL NOT NULL 
      ,TopCover REAL NOT NULL 
      ,Length REAL NOT NULL 
      ,Width REAL NOT NULL 
      ,WidthN INTEGER NOT NULL 
      ))");
    
    auto query = R"(INSERT INTO Belt (
      Serial
      ,PulleyCover 
      ,CordDiameter 
      ,TopCover
      ,Length
      ,Width
      ,WidthN
    ) VALUES (?,?,?,?,?,?,?))"s;
  
    auto [stmt,db] = prepare_query(db_name, query, SQLITE_OPEN_READWRITE | SQLITE_OPEN_NOMUTEX);
    err = sqlite3_bind_text(stmt.get(), 1, belt.Serial.c_str(), belt.Serial.size(), nullptr);
    err = sqlite3_bind_double(stmt.get(), 2, belt.PulleyCover);
    err = sqlite3_bind_double(stmt.get(), 3, belt.CordDiameter);
    err = sqlite3_bind_double(stmt.get(), 4, belt.TopCover);
    err = sqlite3_bind_double(stmt.get(), 5, belt.Length);
    err = sqlite3_bind_double(stmt.get(), 6, belt.Width);
    err = sqlite3_bind_int64(stmt.get(), 7, belt.WidthN);

    tie(err, stmt) = db_step(move(stmt));

    return err == SQLITE_OK || err == SQLITE_DONE;
  }

  std::tuple<cads::Belt,int> fetch_scan_belt(std::string db_name) 
  {
    auto query = R"(SELECT
      Serial
      ,PulleyCover
      ,CordDiameter
      ,TopCover
      ,Length 
      ,Width
      ,WidthN
     FROM Belt)"s;

    auto [stmt,db] = prepare_query(db_name, query);
    auto err = SQLITE_OK;

    tie(err, stmt) = db_step(move(stmt));

    std::tuple<cads::Belt,int> rtn;
    if (err == SQLITE_ROW)
    {
      rtn = {
          {std::string( (const char* )sqlite3_column_text(stmt.get(), 0), sqlite3_column_bytes(stmt.get(), 0)),
          sqlite3_column_double(stmt.get(), 1),
          sqlite3_column_double(stmt.get(), 2),
          sqlite3_column_double(stmt.get(), 3),
          sqlite3_column_double(stmt.get(), 4),
          sqlite3_column_double(stmt.get(), 5),
          sqlite3_column_int64(stmt.get(), 6)},
          0};
    }
    else
    {
      rtn = {cads::Belt{}, -1};
    }

    return rtn;
  }

  bool store_scan_limits(cads::ScanLimits limits, std::string db_name)
  {
    return (bool)create_insert(limits.decompose(),db_name);
  }

  std::expected<cads::ScanLimits,int> fetch_scan_limits(std::string db_name)
  {
    auto row = select_single(cads::ScanLimits().decompose(),db_name);
    if(!row) {
      return std::unexpected(row.error());
    }

    return std::apply([](auto&&... e){return cads::ScanLimits{e...};}, *row);
  }

  bool store_scan_meta(cads::ScanMeta meta, std::string db_name)
  {
    return (bool)create_insert(meta.decompose(),db_name);
  }

  std::expected<cads::ScanMeta,int> fetch_scan_meta(std::string db_name)
  {
    auto row = select_single(cads::ScanMeta().decompose(),db_name);
    if(!row) {
      return std::unexpected(row.error());
    }

    return std::apply([](auto&&... e){return cads::ScanMeta{e...};}, *row);
  }


  bool transfer_profiles(std::string from_db_name, std::string to_db_name, int64_t first_index, int64_t last_index)
  {
    auto err = (int)create_scan_db(to_db_name);
    
    if(err != SQLITE_OK) return false;
    
    auto from_query = R"(SELECT rowid,Y,X,Z FROM Profiles WHERE rowid >= ? AND rowid < ?)";
    auto to_query = R"(INSERT INTO Profiles (Y,X,Z) VALUES (?,?,?))"s;
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
          double y = sqlite3_column_double(from_stmt.get(),1);
          double x = sqlite3_column_double(from_stmt.get(),2);
          z_element *z = (z_element *)sqlite3_column_blob(from_stmt.get(), 3); // Freed on next call to sqlite3_step
          int len = sqlite3_column_bytes(from_stmt.get(), 3) / sizeof(*z);

          auto bind_err = sqlite3_bind_double(to_stmt.get(),2,y); // start from 2 because rowid is 1 and hidden
          bind_err = sqlite3_bind_double(to_stmt.get(),3,x);
          bind_err = sqlite3_bind_blob(to_stmt.get(), 4, z, len, SQLITE_STATIC);
          tie(bind_err, to_stmt) = db_step(move(to_stmt));
          if (bind_err != SQLITE_DONE) {
            return false;
          }
      }
    }while(err == SQLITE_ROW);

    return err == SQLITE_DONE;
    
  }

  bool create_scan_db(std::string db_name) {
    auto err = db_exec(db_name, R"(CREATE TABLE IF NOT EXISTS Profiles (Y REAL NOT NULL, X REAL NOT NULL, Z BLOB NOT NULL))"s);
    return err == SQLITE_OK;
  }

  coro<int, profile, 1> store_scan_coro(std::string db_name)
  {
    spdlog::get("cads")->debug(R"({{func = '{}', msg = '{}' args='{}'}})", __func__,"Entering",db_name);
    
    int err = SQLITE_ERROR;
    {

      create_scan_db(db_name);

      auto query = R"(INSERT INTO Profiles (Y,X,Z) VALUES (?,?,?))"s;
      auto [stmt,db] = prepare_query(db_name, query, SQLITE_OPEN_READWRITE | SQLITE_OPEN_NOMUTEX);
      db_exec(db.get(), R"(PRAGMA synchronous=OFF)"s);

      long zero_seq = 0;

      while (true)
      {
        auto [p, terminate] = co_yield err;

        if (terminate)
          break;

        if(p.z.size() == 0 ) {
          
          if(zero_seq++ == 0) {
            spdlog::get("cads")->error(R"({{func = '{}', msg = '{}'}})", __func__,"No z samples. All sequential no z samples suppressed");
          }
          continue;
        }

        zero_seq = 0;
        auto n = p.z.size() * sizeof(z_element);
        if (n > std::numeric_limits<int>::max())
        {
          terminate = true;
          std::throw_with_nested(std::runtime_error("z data size greater than sqlite limits"));
        }
        
        err = sqlite3_bind_double(stmt.get(), 1, p.y);
        if (err != SQLITE_OK)
        {
          spdlog::get("cads")->error(R"({{func = '{}', fn = '{}', rtn = {}, msg = '', line = {}}})", __func__,"sqlite3_bind_double",err,__LINE__);
        }

        err = sqlite3_bind_double(stmt.get(), 2, p.x_off);
        if (err != SQLITE_OK)
        {
          spdlog::get("cads")->error(R"({{func = '{}', fn = '{}', rtn = {}, msg = '', line = {}}})", __func__,"sqlite3_bind_double",err,__LINE__);
        }

        err = sqlite3_bind_blob(stmt.get(), 3, p.z.data(), (int)n, SQLITE_STATIC);
        if (err != SQLITE_OK)
        {
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

  coro<std::tuple<int, cads::profile>> fetch_scan_coro(long first_index, long last_idx, std::string db_name, int size)
  {
    spdlog::get("cads")->debug(R"({{func = '{}', msg = '{}' args='{}'}})", __func__,"Entering",db_name);
    auto query = R"(SELECT rowid,Y,X,Z FROM PROFILES WHERE rowid >= ? AND rowid < ?)";
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

  std::deque<std::tuple<int, cads::profile>> fetch_scan(long first_index, long last_idx, std::string db_name, int size)
  {
    auto query = R"(SELECT rowid,Y,X,Z FROM Profiles WHERE rowid >= ? AND rowid < ?)";
    auto [stmt,db] = prepare_query(db_name, query);

    auto iend = first_index + size; 
    if(iend > last_idx) iend = last_idx;
    
    auto [p, s] = fetch_scan_coro_step(std::move(stmt), first_index, iend);

    return p;
      
  }


}
