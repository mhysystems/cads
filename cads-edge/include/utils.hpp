#pragma once

#include <string>
#include <functional>
#include <vector>
#include <ranges>
#include <array>

#include <lua.hpp>
#include <date/date.h>
#include <date/tz.h>

#include <coro.hpp>
#include <msg.h>

namespace cads
{
  cads::msgid i_msgid(int);
  coro<cads::msg,cads::msg,1> void_msg();
  coro<std::vector<float>,std::vector<float>,1> file_csv_coro(std::string); 
  void write_vector(std::vector<double> xs,std::string name);
  std::string to_str(date::utc_clock::time_point);
  date::utc_clock::time_point to_clk(std::string);
  std::function<double(double)> mk_online_mean(double mean);
  std::function<double(double)> mk_online_mean();
  double interquartile_mean(std::ranges::subrange<std::vector<float>::iterator>  z) ;
  void dumpstack (lua_State *L);
  std::array<size_t,2> minmin_element(const std::vector<double> & xs);
  std::vector<float> select_if(std::vector<float> a, std::vector<float> b, std::function<bool(float)> c);

}