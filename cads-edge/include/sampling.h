#pragma once

#include <functional>

#include <profile_t.h>

namespace cads
{
  void nan_interpolation_last(z_type &z);
  void nan_interpolation_last(z_type::iterator begin, z_type::iterator end);
  void nan_interpolation_mean(z_type &z);
  void interpolation_nearest(z_type::iterator begin, z_type::iterator end, std::function<bool(z_element)> is);
  z_type interpolate_to_widthn(z_type p, size_t n );
  z_type interpolation_nearest(z_type);
}