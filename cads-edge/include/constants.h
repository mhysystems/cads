#pragma once

#include <cstdint>
#include <cmath>
#include <limits>
#include <string>

#include <date/date.h>
#include <nlohmann/json.hpp>
#include <GoSdk/GoSdkDef.h>

extern nlohmann::json global_config;
constexpr size_t buffer_warning_increment = 4092;

namespace cads {

  using DateTime = std::chrono::time_point<date::local_t,std::chrono::seconds>;

  struct constraints {
    using value_type = std::tuple<double,double>;
    value_type CurrentLength;
    value_type SurfaceSpeed;
    value_type PulleyOcillation;
    value_type CadsToOrigin;
    value_type RotationPeriod;
    value_type BarrelHeight;
    value_type ZUnbiased;
  };

  struct profile_parameters {
    int left_edge_nan;
    int right_edge_nan;
    int spike_filter;
    int sobel_filter;
  };

  struct Conveyor {
    int Id;
    std::string Site;
    std::string Name;
    DateTime Installed;
    std::string Timezone;
    double PulleyCover;
    double CordDiameter;
    double TopCover; 
    double PulleyCircumference;

    operator std::string() const;
  };

  struct webapi_urls {
    using value_type = std::tuple<std::string,bool>;
    value_type add_conveyor;
    value_type add_meta;
    value_type add_belt;
  };

  struct Filters {
    double SchmittThreshold;
  };


  extern constraints global_constraints;
  extern profile_parameters global_profile_parameters;
  extern Conveyor global_conveyor_parameters;
  extern webapi_urls global_webapi;
  extern Filters global_filters;
  
  void init_config(std::string f);
  void drop_config();
  bool between(constraints::value_type range, double value);
  
}

