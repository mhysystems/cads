#pragma once

#include <vector>
#include <tuple>
#include <cstdint>

namespace cads
{
  using z_element = int16_t;
  using z_type = std::vector<z_element>;
  using profile = struct profile{uint64_t y; double x_off; z_type z;}; 

  bool compare_samples(const profile& a, const profile& b, int threshold); 
  std::tuple<z_element,z_element> find_minmax_z(const profile& ps);



  
}