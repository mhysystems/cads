#pragma once

#include <functional>
#include <tuple>
#include <profile.h>
#include <coro.hpp>
#include <constants.h>

namespace cads
{
  std::function<long(double)> mk_pulley_frequency();
  std::function<double(double)> mk_pulley_speed(double init = std::get<0>(global_constraints.SurfaceSpeed));
  std::function<int(z_type &,int,int)> mk_profiles_align(int width_n);
  std::function<double(double)> mk_differentiation(double v0);
  coro<int,std::tuple<double,profile>> encoder_distance_estimation(std::function<void(profile)> next);
  coro<int,std::tuple<double,profile>> encoder_distance_id(std::function<void(profile)> next);

}