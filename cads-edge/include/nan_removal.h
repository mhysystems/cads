#pragma once
#include <vector>
#include <cstdint>

namespace cads {

  std::vector<int16_t> nan_removal(std::vector<int16_t> x, int16_t z_min);

}