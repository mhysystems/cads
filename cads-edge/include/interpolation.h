#pragma once

#include <profile.h>

namespace cads
{
  void nan_interpolation_spline(z_type &z);
  zrange nan_interpolation_spline(zrange z);
  void nan_interpolation_last(z_type &z);
  void nan_interpolation_last(z_type::iterator begin, z_type::iterator end);
  void nan_interpolation_mean(z_type &z);
  void interpolation_nearest(z_type::iterator begin, z_type::iterator end, std::function<bool(z_element)> is);
  decltype(cads::profile::z) interpolate_to_widthn(decltype(cads::profile::z) p, size_t n );
  decltype(cads::profile::z) interpolation_nearest(decltype(cads::profile::z));
}