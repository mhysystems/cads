#pragma once

#include <cstdint>
#include <cmath>
#include <limits>
#include <string>
#include <atomic>
#include <filesystem>
#include <optional>

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
    std::string Site;
    std::string Name;
    std::string Timezone;
    double PulleyCircumference;
    double TypicalSpeed;
    operator std::string() const;
  };

  struct Belt {
    std::string Serial;
    double PulleyCover;
    double CordDiameter;
    double TopCover; 
    double Length;
    double Width;
    int64_t WidthN;
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

  struct Databases {
    std::string profile_db_name;
    std::string state_db_name;
    std::string transient_db_name;
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
  extern Databases database_names;
  extern HeartBeat constants_heartbeat;
  extern std::atomic<bool> terminate_signal;
  extern std::optional<std::string> luascript_name;
  
  void init_config(std::string f);
  void drop_config();
  bool between(std::tuple<double,double> range, double value);
  
}

