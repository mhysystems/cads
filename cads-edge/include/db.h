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
#include <msg.h>


namespace cads
{

  void create_default_dbs();

  // profile db
  // profile table
  coro<int, std::tuple<int, int, profile>, 1> store_profile_coro(std::string name = "");
  coro<std::tuple<int, profile>> fetch_belt_coro(int revid, long last_idx, long first_idx = 0, int size = 256, std::string name = "");
  long count_with_width_n(std::string name, int revid, int width_n);
  int store_profile_parameters(GocatorProperties p, std::string name = "");
  std::tuple<cads::GocatorProperties, int> fetch_profile_parameters(std::string name = "");
  std::tuple<double,double,double,double,int> fetch_belt_dimensions(int revid, int idx, std::string name);


  // state db
  // state table
  date::utc_clock::time_point fetch_daily_upload(std::string name = "");
  int store_daily_upload(date::utc_clock::time_point date, std::string name = "");
  
  std::tuple<int,bool> fetch_conveyor_id(std::string name = "");
  void store_conveyor_id(int id, std::string name = "");

  std::tuple<int,bool> fetch_belt_id(std::string name = "");
  void store_belt_id(int id, std::string name = "");

  std::tuple<date::utc_clock::time_point,std::vector<double>> fetch_last_motif(std::string name = "");
  bool store_motif_state(std::tuple<date::utc_clock::time_point,std::vector<double>> row, std::string db_name = "");

  // scans table
  namespace state {
    //using scan = std::tuple<date::utc_clock::time_point,std::string,int64_t,int64_t>;
    struct scan {
      date::utc_clock::time_point scanned_utc;
      std::string db_name;
      std::string url;
      int64_t begin_index;
      int64_t cardinality;
      int64_t uploaded;
      int64_t status;
      int64_t conveyor_id;
    };
    //enum scani {ScanBegin, Path, Uploaded, Status};
  }
  std::deque<cads::state::scan> fetch_scan_state(std::string name ="");
  bool store_scan_state(cads::state::scan scan, std::string db_name = "");
  bool delete_scan_state(cads::state::scan scan, std::string db_name = "");
  bool update_scan_state(cads::state::scan scan, std::string db_name = "");
  coro<std::tuple<int, z_type>> fetch_scan_coro(long first_index, long last_idx, std::string db_name, int size = 256);


  // scan db
  bool create_scan_db(std::string db_name);
  bool transfer_profiles(std::string from_db_name, std::string to_db_name, int64_t first_index, int64_t last_index = std::numeric_limits<int64_t>::max());
  bool store_scan_gocator(cads::GocatorProperties gocator, std::string db_name);
  bool store_scan_properties(std::tuple<date::utc_clock::time_point,date::utc_clock::time_point> props, std::string db_name);
  long zs_count(std::string db_name);
  coro<int, z_type, 1> store_scan_coro(std::string db_name);
  std::tuple<cads::GocatorProperties,int> fetch_scan_gocator(std::string db_name);
  


  // transients
  coro<int, double, 1> store_last_y_coro(std::string name = "");  
  void store_errored_profile(const cads::z_type &z, std::string id, std::string name ="");

}
