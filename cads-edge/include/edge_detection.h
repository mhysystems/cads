#pragma once

#include <profile.h>
#include <tuple>

namespace cads 
{
  std::tuple<int,int> find_profile_edges_sobel(const z_type& z, int len, int x_width);
  std::tuple<int,int> find_profile_edges_nans(const z_type& z, int len);
} // namespace cads