#pragma once

#include <cstdint>
#include <cmath>
#include <limits>
#include <string>

#include <nlohmann/json.hpp>
#include <GoSdk/GoSdkDef.h>

extern nlohmann::json global_config;

namespace cads {

  template<typename T> struct NaN;

  template<> struct NaN<int16_t> {
    static constexpr k16s value = k16S_NULL;
    static bool isnan(k16s a) {
      return a == value;
    }
  };

  template<> struct NaN<float> {
    static constexpr float value = std::numeric_limits<float>::quiet_NaN();
    static bool isnan(float a) {
      return std::isnan(a);
    }
  };

  void init_config(std::string f);
  void drop_config();
}

