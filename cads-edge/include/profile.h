#pragma once

#include <vector>
#include <tuple>
#include <cstdint>
#include <deque>

namespace cads
{
  using z_element = int16_t;
  using y_type = double;
  using z_type = std::vector<z_element>;
  using profile = struct profile{y_type y; double x_off; z_type z;}; 
  using gocator_profile = std::tuple<y_type,double,std::vector<z_element>>;
  using profile_window = std::deque<profile>;

  bool compare_samples(const profile& a, const profile& b, int threshold); 
  std::tuple<z_element,z_element> find_minmax_z(const profile& ps);
  std::tuple<z_element,z_element> barrel_offset(const z_type& win, double z_resolution, double z_height_mm);



  
}