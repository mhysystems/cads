#pragma once

#include <cstdint>
#include <cmath>
#include <limits>
#include <string>

#include <date/date.h>
#include <nlohmann/json.hpp>
#include <GoSdk/GoSdkDef.h>

extern nlohmann::json global_config;
constexpr size_t buffer_warning_increment = 4096;

namespace cads {

  using DateTime = std::chrono::time_point<date::local_t,std::chrono::seconds>;

  struct SqliteGocatorConfig {
    using range_type = std::tuple<long,long>;
    range_type range;
    double fps;
    bool forever;
    double delay;
  };
  
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
    int64_t Id;
    std::string Site;
    std::string Name;
    std::string Timezone;
    double PulleyCircumference;
    int64_t Belt;

    operator std::string() const;
  };

  struct Belt {
    int64_t Id;
    DateTime Installed;
    double PulleyCover;
    double CordDiameter;
    double TopCover; 
    double Length;
    double Width;
    double Splices;
    int64_t Conveyor;

    operator std::string() const;
  };

  struct Scan {
    int32_t Orientation;

    operator std::string() const;
  };

  struct webapi_urls {
    using value_type = std::tuple<std::string,bool>;
    value_type add_conveyor;
    value_type add_meta;
    value_type add_belt;
    value_type add_scan;
  };

  struct Filters {
    double SchmittThreshold;
  };

  struct Dbscan {
    double InCluster;
    size_t MinPoints;
  };


  extern constraints global_constraints;
  extern profile_parameters global_profile_parameters;
  extern Conveyor global_conveyor_parameters;
  extern webapi_urls global_webapi;
  extern Filters global_filters;
  extern SqliteGocatorConfig sqlite_gocator_config;
  extern Dbscan dbscan_config;
  
  void init_config(std::string f);
  void drop_config();
  bool between(constraints::value_type range, double value);
  
}

