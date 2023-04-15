#pragma once

#include <functional>
#include <tuple>
#include <profile.h>
#include <coro.hpp>
#include <constants.h>
#include <chrono>

namespace cads
{
  using PulleyRevolution = std::tuple<bool, double, std::chrono::duration<double>>;

  std::function<PulleyRevolution(double)> mk_pulley_revolution();
  std::function<std::tuple<double,double,double>(PulleyRevolution,double)> mk_pulley_stats(double init = std::get<0>(global_constraints.SurfaceSpeed));
  std::function<int(z_type &,int,int)> mk_profiles_align(int width_n);
  coro<int,std::tuple<PulleyRevolution,profile>> encoder_distance_estimation(std::function<void(profile)> next, double stide);
  coro<int,std::tuple<double,profile>> encoder_distance_id(std::function<void(profile)> next);

}