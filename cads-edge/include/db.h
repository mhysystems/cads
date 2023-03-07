#pragma once

#include <tuple>
#include <chrono>
#include <cstdint>
#include <string>

#include <sqlite3.h>
#include <date/date.h>
#include <readerwriterqueue.h>

#include <window.hpp>
#include <profile.h>
#include <coro.hpp>

namespace cads
{

  bool create_profile_db(std::string name = "");
  bool create_program_state_db(std::string name = "");
  bool create_transient_db(std::string name = "");
  void create_default_dbs();

  coro<int, std::tuple<int, int, profile>, 1> store_profile_coro(std::string name = "");
  coro<int, double, 1> store_last_y_coro(std::string name = "");
  coro<std::tuple<int, profile>> fetch_belt_coro(int revid, long last_idx, long first_idx = 0, int size = 256, std::string name = "");
  long count_with_width_n(std::string name, int revid, int width_n);
  int store_profile_parameters(profile_params p, std::string name = "");
  std::tuple<profile_params, int> fetch_profile_parameters(std::string name);

  std::tuple<double,double,double,double,int> fetch_belt_dimensions(int revid, int idx, std::string name);

  std::chrono::time_point<date::local_t, std::chrono::seconds> fetch_daily_upload(std::string name);
  int store_daily_upload(std::chrono::time_point<date::local_t, std::chrono::seconds> date, std::string name);
  
  std::tuple<int,bool> fetch_conveyor_id(std::string name);
  void store_conveyor_id(int id, std::string name);
  void store_errored_profile(const cads::z_type &z, std::string name ="");


}
