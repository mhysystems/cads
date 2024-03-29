#pragma once

#include <functional>
#include <tuple>
#include <chrono>

#include <profile_t.h>
#include <coro.hpp>
#include <constants.h>
#include <io.hpp>
#include <msg.h>

namespace cads
{
  using PulleyRevolution = std::tuple<bool, double>;

  std::function<PulleyRevolution(double)> mk_pulley_revolution(RevolutionSensorConfig);
  std::function<PulleyRevolution(double)> mk_pseudo_revolution(double);
  std::function<std::tuple<double,double,double>(PulleyRevolution,double,std::chrono::time_point<std::chrono::system_clock>)> mk_pulley_stats(double,double);
  coro<msg,msg,1> encoder_distance_estimation(cads::Io<msg> &next, double stide);
  coro<int,std::tuple<double,profile>> encoder_distance_id(std::function<void(profile)> next);

}