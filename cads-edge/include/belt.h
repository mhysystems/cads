#pragma once

#include <functional>
#include <profile.h>

namespace cads
{
  std::function<long(double)> mk_pulley_frequency();
  std::function<int(z_type &,int,int)> mk_profiles_align(int width_n);
}