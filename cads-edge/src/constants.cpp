#include <tuple>
#include <algorithm> 
#include <sstream>
#include <filesystem>
#include <signal.h> 

#include <date/tz.h>
#include <lua.hpp>

#include <constants.h>
#include <init.h>

nlohmann::json global_config;
using Lua = std::unique_ptr<lua_State, decltype(&lua_close)>;

namespace {

  auto mk_webapi_urls(nlohmann::json config) {
    using namespace std;
    
    auto add_conveyor = config["webapi_urls"]["add_conveyor"].get<cads::webapi_urls::value_type>();
    auto add_meta = config["webapi_urls"]["add_meta"].get<cads::webapi_urls::value_type>();
    auto add_belt = config["webapi_urls"]["add_belt"].get<cads::webapi_urls::value_type>();
    auto add_scan = config["webapi_urls"]["add_scan"].get<cads::webapi_urls::value_type>();

    return cads::webapi_urls{add_conveyor,add_meta,add_belt,add_scan};

  }

  auto mk_communications(nlohmann::json config) {
    using namespace std;
    
    auto NatsUrl = config["communications"]["NatsUrl"].get<std::string>();
    auto UploadRows = config["communications"]["UploadRows"].get<size_t>();

    return cads::Communications{NatsUrl,UploadRows};

  }

  auto mk_fiducial(nlohmann::json config) {

    double fiducial_depth = config["fiducial"]["fiducial_depth"].get<double>();
    double fiducial_x = config["fiducial"]["fiducial_x"].get<double>();
    double fiducial_y = config["fiducial"]["fiducial_y"].get<double>();
    double fiducial_gap = config["fiducial"]["fiducial_gap"].get<double>();
    double edge_height = config["fiducial"]["edge_height"].get<double>();


    return cads::Fiducial{fiducial_depth,fiducial_x,fiducial_y,fiducial_gap,edge_height};
  }

  auto mk_origin_detection(nlohmann::json config) {

    auto belt_length = config["OriginDetection"]["BeltLength"].get<cads::OriginDetection::value_type>();
    auto cross_correlation_threshold = config["OriginDetection"]["cross_correlation_threshold"].get<double>();
    bool  dump_match = config["OriginDetection"]["dump_match"].get<bool>();

    return cads::OriginDetection{belt_length,cross_correlation_threshold,dump_match};
  }

  auto mk_device(nlohmann::json config) {
    
    auto serial = config["Device"]["Serial"].get<long>();

    return cads::Device{serial};
  }

  auto mk_upload(nlohmann::json config) {
    
    auto period = config["upload"]["Period"].get<long>();

    return cads::UploadConstants{std::chrono::seconds(period)};
  }

  auto mk_heartbeat(nlohmann::json config) {

    if(!config.contains("heartbeat"))
    {
      return cads::HeartBeat{false,"",std::chrono::milliseconds(1)};
    }

    auto SendHeartBeat = config["heartbeat"]["SendHeartBeat"].get<bool>();
    auto Subject = config["heartbeat"]["Subject"].get<std::string>();
    auto Period =  config["heartbeat"]["Period_ms"].get<long>();

    return cads::HeartBeat{SendHeartBeat,Subject,std::chrono::milliseconds(Period)};
  }

  void sigint_handler([[maybe_unused]]int s) {
    cads::terminate_signal = true;
  }
}

namespace cads {  

  Device constants_device;
  webapi_urls global_webapi;
  Communications communications_config;
  Fiducial fiducial_config;
  OriginDetection config_origin_detection;
  AnomalyDetection anomalies_config;
  UploadConstants constants_upload;
  HeartBeat constants_heartbeat;

  std::atomic<bool> terminate_signal = false;

    
  void init_config(std::string f) {
    struct sigaction sigIntHandler;

    sigIntHandler.sa_handler = ::sigint_handler;
    sigemptyset(&sigIntHandler.sa_mask);
    sigIntHandler.sa_flags = 0;

    sigaction(SIGINT, &sigIntHandler, NULL);
    
    
    auto json = slurpfile(f);
		auto config = nlohmann::json::parse(json);
    constants_device = mk_device(config);
    global_webapi = mk_webapi_urls(config);
    communications_config = mk_communications(config);
    fiducial_config = mk_fiducial(config);
    config_origin_detection = mk_origin_detection(config);
    constants_upload = mk_upload(config);
    constants_heartbeat = mk_heartbeat(config);
    global_config = config;
  }

  void drop_config() {
    global_config = nlohmann::json();
  }

  bool between(std::tuple<double,double> range, double value) {
    return get<0>(range) <= value && value <= get<1>(range);  
  }

  Conveyor::operator std::string() const {
    
    nlohmann::json params_json;
  
    params_json["Id"] = Id;
    params_json["Org"] = Org;
    params_json["Site"] = Site;
    params_json["Name"] = Name;
    params_json["Timezone"] = Timezone;
    params_json["PulleyCircumference"] = PulleyCircumference;
    params_json["Belt"] = Belt;
    params_json["Length"] = Length;

    return params_json.dump();
  }

  Belt::operator std::string() const {
    
    nlohmann::json params_json;
  
    params_json["Id"] = Id;
    params_json["Installed"] =  date::format("%FT%TZ", Installed);
    params_json["PulleyCover"] = PulleyCover;
    params_json["CordDiameter"] = CordDiameter;
    params_json["TopCover"] = TopCover;
    params_json["Length"] = Length;
    params_json["Width"] = Width;
    params_json["Splices"] = Splices;
    params_json["Conveyor"] = Conveyor;
    
    return params_json.dump();
  }

  Scan::operator std::string() const {
    
    nlohmann::json params_json;
  
    params_json["Orientation"] = Orientation;
    
    return params_json.dump();
  }


}