#pragma once

#include <vector>
#include <tuple>
#include <cstdint>
#include <deque>
#include <limits>
#include <cmath>

namespace cads
{
  using z_element = float; //int16_t;
  using y_type = double;
  using z_type = std::vector<z_element>;
  using profile = struct {y_type y; double x_off; z_type z;}; 
  using profile_window = std::deque<profile>;

  const cads::profile null_profile{std::numeric_limits<cads::y_type>::max(),std::numeric_limits<double>::max(),{}};

  bool compare_samples(const profile& a, const profile& b, int threshold); 
  std::tuple<z_element,z_element> find_minmax_z(const profile& ps);
  std::tuple<z_element,z_element,bool> barrel_offset(const z_type& win,double z_height_mm);
  
}