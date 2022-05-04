#pragma once
#include <cstdint>
#include <profile.h>
#include <cmath>

namespace cads {
  constexpr int16_t InvalidRange16Bit = 0x8000;
  template<typename T> struct NaN;

  template<> struct NaN<int16_t> {
    static constexpr int16_t value = InvalidRange16Bit;
  };

  template<> struct NaN<float> {
    static constexpr float value = NAN;
  };
}

