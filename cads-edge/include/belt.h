#pragma once

#include <functional>
#include <tuple>
#include <profile.h>
#include <coro.hpp>
#include <constants.h>
#include <chrono>

namespace cads
{
  using PulleyRevolution = std::tuple<bool, double>;

  std::function<PulleyRevolution(double)> mk_pulley_revolution(double);
  std::function<std::tuple<double,double,double>(PulleyRevolution,double)> mk_pulley_stats(double init = global_conveyor_parameters.MaxSpeed);
  std::function<int(z_type &,int,int)> mk_profiles_align(int width_n);
  coro<int,std::tuple<PulleyRevolution,profile>> encoder_distance_estimation(std::function<void(profile)> next, double stide);
  coro<int,std::tuple<double,profile>> encoder_distance_id(std::function<void(profile)> next);

}