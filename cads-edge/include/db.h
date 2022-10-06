#pragma once

#include <tuple>
#include <sqlite3.h>
#include <cstdint>
#include <string>

#include <readerwriterqueue.h>

#include <window.hpp>
#include <profile.h>
#include <coro.hpp>

namespace cads
{

  bool create_db(std::string name = "");
  coro<int, std::tuple<int, int, profile>, 1> store_profile_coro(std::string name = "");
  coro<std::tuple<int, profile>> fetch_belt_coro(int revid, int last_idx, int size = 256, std::string name = "");

  int store_profile_parameters(profile_params p, std::string name = "");
  std::tuple<profile_params, int> fetch_profile_parameters(std::string name);

  std::tuple<double,double,double,double,int> fetch_belt_dimensions(int revid, std::string name);

}
