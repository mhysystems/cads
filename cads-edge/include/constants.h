#pragma once

#include <cstdint>
#include <cmath>
#include <limits>
#include <string>
#include <atomic>

#include <date/date.h>
#include <nlohmann/json.hpp>
#include <GoSdk/GoSdkDef.h>
#include <measurements.h>

extern nlohmann::json global_config;
constexpr size_t buffer_warning_increment = 4096;

namespace cads {

  using ScanParameters = struct {
    double Length;
    double Width;
  };

  using DateTime = std::chrono::time_point<date::local_t,std::chrono::seconds>;

  struct SqliteGocatorConfig {
    using range_type = std::tuple<long,long>;
    range_type range;
    double fps;
    bool forever;
    double delay;
  };
  

  struct profile_parameters {
    int left_edge_nan;
    int right_edge_nan;
    int spike_filter;
    int sobel_filter;
  };

  struct Conveyor {
    int64_t Id;
    std::string Org;
    std::string Site;
    std::string Name;
    std::string Timezone;
    double PulleyCircumference;
    double TypicalSpeed;
    int64_t Belt;
    double Length;
    int64_t WidthN;
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
    double WidthN;
    int64_t Splices;
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

  struct Device {
    long Serial;
  };

  struct Dbscan {
    double InCluster;
    size_t MinPoints;
  };

  struct RevolutionSensor{
    enum class Source {height_raw, height_filtered, length};
    Source source;
    size_t trigger_num;
    double bias;
    bool bidirectional;
  };

  struct Communications {
    std::string NatsUrl;
    size_t UploadRows;
  };

  struct Fiducial {
    double fiducial_depth;
    double fiducial_x;
    double fiducial_y;
    double fiducial_gap;

  };

  struct OriginDetection {
    using value_type = std::tuple<double,double>;
    value_type belt_length;
    double cross_correlation_threshold;
    bool   dump_match;
  };

  struct AnomalyDetection {
    size_t WindowSize;
    size_t BeltPartitionSize;
    size_t BeltSize;
    size_t MinPosition;
    size_t MaxPosition;
  };

  struct GocatorConstants {
    double Fps;
  };

  struct UploadConstants {
    std::chrono::seconds Period;
  };


  extern Device constants_device;
  extern profile_parameters global_profile_parameters;
  extern Conveyor global_conveyor_parameters;
  extern Belt global_belt_parameters;
  extern webapi_urls global_webapi;
  extern Filters global_filters;
  extern SqliteGocatorConfig sqlite_gocator_config;
  extern Dbscan dbscan_config;
  extern RevolutionSensor revolution_sensor_config;
  extern Communications communications_config;
  extern Fiducial fiducial_config;
  extern OriginDetection config_origin_detection;
  extern Measure measurements;
  extern GocatorConstants constants_gocator;
  extern UploadConstants constants_upload;

  extern std::atomic<bool> terminate_signal;
  
  void init_config(std::string f);
  void drop_config();
  bool between(std::tuple<double,double> range, double value);
  
}

