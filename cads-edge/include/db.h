#pragma once

#include <tuple>
#include <chrono>
#include <cstdint>
#include <string>
#include <deque>

#include <sqlite3.h>
#include <date/date.h>
#include <date/tz.h>
#include <readerwriterqueue.h>

#include <profile.h>
#include <coro.hpp>

namespace cads
{

  void create_default_dbs();

  // profile db
  // profile table
  coro<int, std::tuple<int, int, profile>, 1> store_profile_coro(std::string name = "");
  coro<std::tuple<int, profile>> fetch_belt_coro(int revid, long last_idx, long first_idx = 0, int size = 256, std::string name = "");
  long count_with_width_n(std::string name, int revid, int width_n);
  int store_profile_parameters(profile_params p, std::string name = "");
  std::tuple<profile_params, int> fetch_profile_parameters(std::string name);
  std::tuple<double,double,double,double,int> fetch_belt_dimensions(int revid, int idx, std::string name);


  // state db
  // state table
  date::utc_clock::time_point fetch_daily_upload(std::string name = "");
  int store_daily_upload(date::utc_clock::time_point date, std::string name = "");
  
  std::tuple<int,bool> fetch_conveyor_id(std::string name = "");
  void store_conveyor_id(int id, std::string name = "");

  std::tuple<int,bool> fetch_belt_id(std::string name = "");
  void store_belt_id(int id, std::string name = "");

  // scans table
  namespace state {
    using scan = std::tuple<date::utc_clock::time_point,std::string,int64_t,int64_t>;
  }
  std::deque<cads::state::scan> fetch_scan_state(std::string name ="");
  bool store_scan_state(std::string scan_db, std::string db_name = "");
  void store_scan_status(int64_t status, std::string scan_name, std::string name = "");
  void store_scan_uploaded(int64_t idx, std::string scan_name, std::string name = "");
  bool delete_scan_state(std::string Path, std::string db_name = "");
  std::tuple<int64_t,bool> fetch_scan_uploaded(std::string scan_name, std::string name = "");
  coro<std::tuple<int, z_type>> fetch_scan_coro(long last_idx, long first_index, std::string db_name, int size = 256);


  // scan db
  bool store_scan_gocator(std::tuple<double,double,double,double> gocator, std::string db_name);
  bool store_scan_properties(std::tuple<date::utc_clock::time_point,date::utc_clock::time_point> props, std::string db_name);
  long zs_count(std::string db_name);
  coro<int, z_type, 1> store_scan_coro(std::string db_name);


  // transients
  coro<int, double, 1> store_last_y_coro(std::string name = "");  
  void store_errored_profile(const cads::z_type &z, std::string name ="");

}
