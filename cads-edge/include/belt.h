#pragma once

#include <functional>
#include <tuple>
#include <chrono>

#include <profile.h>
#include <coro.hpp>
#include <constants.h>
#include <io.hpp>
#include <msg.h>

namespace cads
{
  using PulleyRevolution = std::tuple<bool, double>;

  std::function<PulleyRevolution(double)> mk_pulley_revolution();
  std::function<PulleyRevolution(double)> mk_pseudo_revolution();
  std::function<std::tuple<double,double,double>(PulleyRevolution,double,std::chrono::time_point<std::chrono::system_clock>)> mk_pulley_stats(double init = global_conveyor_parameters.TypicalSpeed);
  std::function<int(z_type &,int,int)> mk_profiles_align(int width_n);
  coro<msg,msg> encoder_distance_estimation(cads::Io &next, double stide);
  coro<int,std::tuple<double,profile>> encoder_distance_id(std::function<void(profile)> next);

}