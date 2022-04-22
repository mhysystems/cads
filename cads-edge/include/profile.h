#pragma once

#include <vector>
#include <cstdint>

namespace cads
{
  using profile = struct profile{uint64_t y; double x_off; std::vector<int16_t> z;};  
}