#pragma once

#include <cstdint>
#include <cmath>

#include <nlohmann/json.hpp>
#include <GoSdk/GoSdkDef.h>

extern nlohmann::json global_config;

namespace cads {

  template<typename T> struct NaN;

  template<> struct NaN<k16s> {
    static constexpr k16s value = k16S_NULL;
  };

  template<> struct NaN<float> {
    static constexpr float value = NAN;
  };
}

