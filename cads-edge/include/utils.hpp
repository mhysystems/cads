#pragma once

#include <string>
#include <functional>
#include <vector>
#include <ranges>

#include <lua.hpp>
#include <date/date.h>
#include <date/tz.h>

namespace cads
{
  std::string to_str(date::utc_clock::time_point);
  date::utc_clock::time_point to_clk(std::string);
  std::function<double(double)> mk_online_mean(double mean = 0);
  double interquartile_mean(std::ranges::subrange<std::vector<float>::iterator>  z) ;
  void dumpstack (lua_State *L);
}