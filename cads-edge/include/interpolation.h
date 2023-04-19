#pragma once

#include <profile.h>

namespace cads
{
  void nan_interpolation_spline(z_type &z);
  zrange nan_interpolation_spline(zrange z);
  void nan_interpolation_last(z_type &z);
  void nan_interpolation_last(z_type::iterator begin, z_type::iterator end);
  void nan_interpolation_mean(z_type &z);
}