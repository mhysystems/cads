#pragma once

#include <profile.h>

namespace cads
{
  void nan_interpolation_spline2(z_type &z);
  //void nan_interpolation_spline(std::ranges::range auto &z);
  void nan_interpolation_last(z_type &z);
  void nan_interpolation_last(z_type::iterator begin, z_type::iterator end);
}