#pragma once

#include <vector>
#include <cstdint>

namespace cads
{
  using z_element = int16_t;
  using z_type = std::vector<z_element>;
  using profile = struct profile{uint64_t y; double x_off; z_type z;};  
}