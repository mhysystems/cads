#pragma once

#include <cstdint>
#include <cmath>
#include <limits>
#include <string>
#include <atomic>
#include <filesystem>

#include <date/date.h>
#include <nlohmann/json.hpp>
#include <GoSdk/GoSdkDef.h>

extern nlohmann::json global_config;
constexpr size_t buffer_warning_increment = 4096;

namespace cads {


  using DateTime = std::chrono::time_point<date::local_t,std::chrono::seconds>;

  struct HeartBeat {
    bool SendHeartBeat;
    std::string Subject;
    std::chrono::milliseconds Period;
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

  struct Device {
    long Serial;
  };

  struct RevolutionSensorConfig{
    enum class Source {height_raw, height_filtered, length};
    Source source;
    double trigger_distance;
    double bias;
    double threshold;
    bool bidirectional;
    long long skip;
  };

  struct Communications {
    std::string NatsUrl;
    size_t UploadRows;
  };

  struct webapi_urls {
    using value_type = std::tuple<std::string,bool>;
    value_type add_meta;
    value_type add_belt;
  };

  struct UploadConfig {
    std::chrono::seconds Period;
    webapi_urls urls;
  };

  extern Device constants_device;
  extern Communications communications_config;
  extern UploadConfig upload_config;
  extern HeartBeat constants_heartbeat;
  extern std::atomic<bool> terminate_signal;
  
  void init_config(std::string f);
  void drop_config();
  bool between(std::tuple<double,double> range, double value);
  
}

