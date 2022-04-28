#pragma once
#include <profile.h>

namespace cads
{
  void spike_filter(z_type& z, int window_size);
  void nan_filter(z_type& z);
  void barrel_height_compensate(z_type& z, z_element z_off);
} // namespace cads
