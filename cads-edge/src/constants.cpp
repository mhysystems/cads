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

namespace {

  auto mk_sqlite_gocator(nlohmann::json config) {
    auto current_length = config["sqlite_gocator"]["range"].get<cads::SqliteGocatorConfig::range_type>();
    auto fps = config["sqlite_gocator"]["fps"].get<double>();
    auto forever = config["sqlite_gocator"]["forever"].get<bool>();
    auto delay = config["sqlite_gocator"]["delay"].get<double>();

    return cads::SqliteGocatorConfig{current_length,fps,forever,delay};
  }

  auto mk_profile_parameters(nlohmann::json config) {
    using namespace std;
    auto left_edge_nan = config["left_edge_nan"].get<int>();
    auto right_edge_nan = config["right_edge_nan"].get<int>();
    auto spike_filter = config["spike_filter"].get<int>();
    auto sobel_filter = config["sobel_filter"].get<int>();

    return cads::profile_parameters{left_edge_nan,right_edge_nan,spike_filter,sobel_filter};

  }

  auto mk_conveyor_parameters(nlohmann::json config) {

    int64_t Id = 0;
    auto Org = config["conveyor"]["Org"].get<std::string>();
    auto Site = config["conveyor"]["Site"].get<std::string>();
    auto Name = config["conveyor"]["Name"].get<std::string>();
    auto Timezone = config["conveyor"]["Timezone"].get<std::string>();

    auto PulleyCircumference = config["conveyor"]["PulleyCircumference"].get<double>();
    auto MaxSpeed = config["conveyor"]["MaxSpeed"].get<double>();
    
    int64_t Belt = 0;

    return cads::Conveyor{Id,Org,Site,Name,Timezone,PulleyCircumference,MaxSpeed,Belt};

  }

  auto mk_belt_parameters(nlohmann::json config) {

    int64_t Id = 0;
    cads::DateTime Installed;
    std::istringstream in(config["belt"]["Installed"].get<std::string>());
    in >> date::parse("%FT%TZ", Installed);
    auto PulleyCover = config["belt"]["PulleyCover"].get<double>();
    auto CordDiameter = config["belt"]["CordDiameter"].get<double>();
    auto TopCover = config["belt"]["TopCover"].get<double>();
    auto Length = config["belt"]["Length"].get<double>();
    auto Width = config["belt"]["Width"].get<double>();
    auto WidthN = config["belt"]["WidthN"].get<double>();
    auto Splices = config["belt"]["Splices"].get<int64_t>();
    int64_t Conveyor = 0;

    return cads::Belt{Id,Installed,PulleyCover,CordDiameter,TopCover,Length,Width,WidthN,Splices,Conveyor};

  }

  auto mk_scan_parameters(nlohmann::json config) {

    auto Orientation = config["scan"]["Orientation"].get<int32_t>();

    return cads::Scan{Orientation};

  }


  auto mk_webapi_urls(nlohmann::json config) {
    using namespace std;
    
    auto add_conveyor = config["webapi_urls"]["add_conveyor"].get<cads::webapi_urls::value_type>();
    auto add_meta = config["webapi_urls"]["add_meta"].get<cads::webapi_urls::value_type>();
    auto add_belt = config["webapi_urls"]["add_belt"].get<cads::webapi_urls::value_type>();
    auto add_scan = config["webapi_urls"]["add_scan"].get<cads::webapi_urls::value_type>();

    return cads::webapi_urls{add_conveyor,add_meta,add_belt,add_scan};

  }

  auto mk_filters(nlohmann::json config) {
    
    auto SchmittThreshold = config["filters"]["SchmittThreshold"].get<double>();
    auto LeftDamp = config["filters"]["LeftDamp"].get<std::vector<float>>();
    auto LeftDampOff =  config["filters"]["LeftDampOff"].get<float>();

    return cads::Filters{SchmittThreshold,LeftDamp,LeftDampOff};

  }

  auto mk_dbscan(nlohmann::json config) {
    auto InCluster = config["dbscan"]["InCluster"].get<double>();
    auto MinPoints = config["dbscan"]["MinPoints"].get<size_t>();
    return cads::Dbscan{InCluster,MinPoints};

  }

  auto mk_revolution_sensor(nlohmann::json config) {
    auto source_s = config["revolution_sensor"]["source"].get<std::string>();
    cads::RevolutionSensor::Source source;
    if(source_s == "raw") {
      source = cads::RevolutionSensor::Source::height_raw;
    }else if(source_s == "length") {
      source = cads::RevolutionSensor::Source::length;
    }else {
      source = cads::RevolutionSensor::Source::height_filtered;  
    }

    auto trigger_num = config["revolution_sensor"]["trigger_num"].get<size_t>();
    auto bias = config["revolution_sensor"]["bias"].get<double>();
    auto bidirectional = config["revolution_sensor"]["bidirectional"].get<bool>();

    return cads::RevolutionSensor{source,trigger_num,bias,bidirectional};

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


    return cads::Fiducial{fiducial_depth,fiducial_x,fiducial_y,fiducial_gap};
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
  
  void sigint_handler([[maybe_unused]]int s) {
    cads::terminate_signal = true;
  }
}

namespace cads {  

  Device constants_device;
  profile_parameters global_profile_parameters;
  Conveyor global_conveyor_parameters;
  Belt global_belt_parameters;
  Scan global_scan_parameters;
  webapi_urls global_webapi;
  Filters global_filters;
  SqliteGocatorConfig sqlite_gocator_config;
  Dbscan dbscan_config;
  RevolutionSensor revolution_sensor_config;
  Communications communications_config;
  Fiducial fiducial_config;
  OriginDetection config_origin_detection;
  Measure measurements;
  AnomalyDetection anomalies_config;
  GocatorConstants constants_gocator;
  UploadConstants constants_upload;

  std::atomic<bool> terminate_signal = false;

  int lua_transition(std::string f) {
    
    using Lua = std::unique_ptr<lua_State, decltype(&lua_close)>;
    namespace fs = std::filesystem;

    fs::path luafile{f};
    luafile.replace_extension("lua");

    auto L = Lua{luaL_newstate(),lua_close};
    luaL_openlibs( L.get() );

    auto lua_status = luaL_dofile(L.get(),luafile.string().c_str());
    
    if(lua_status != LUA_OK) {
      return -1;
    }

    AnomalyDetection config;

    lua_status = lua_getglobal(L.get(),"anomaly");

    if (lua_istable(L.get(), -1)) { 
        lua_pushstring(L.get(), "WindowLength"); 
        lua_gettable(L.get(), -2); 

        if (lua_isnumber(L.get(), -1)) { 
          config.WindowLength = lua_tonumber(L.get(), -1); 
          lua_pop(L.get(),1);
        }

        lua_pushstring(L.get(), "BeltPartitionLength"); 
        lua_gettable(L.get(), -2); 

        if (lua_isnumber(L.get(), -1)) { 
          config.BeltPartitionLength = lua_tonumber(L.get(), -1); 
          lua_pop(L.get(),1);
        }
    }

    anomalies_config = config;

    Belt belt;
    belt.Conveyor = 0;
    belt.Id = 0;

    lua_status = lua_getglobal(L.get(),"belt");

    if(lua_istable(L.get(),-1)) {
      lua_getfield(L.get(), -1, "Installed");
      std::istringstream in(lua_tostring(L.get(), -1));
      lua_pop(L.get(), 1);
      in >> date::parse("%FT%TZ", belt.Installed);
    
      lua_getfield(L.get(), -1, "PulleyCover");
      belt.PulleyCover = lua_tonumber(L.get(), -1);
      lua_pop(L.get(), 1);

      lua_getfield(L.get(), -1, "CordDiameter");
      belt.CordDiameter = lua_tonumber(L.get(), -1);
      lua_pop(L.get(), 1);

      lua_getfield(L.get(), -1, "TopCover");
      belt.TopCover = lua_tonumber(L.get(), -1);
      lua_pop(L.get(), 1);

      lua_getfield(L.get(), -1, "Width");
      belt.Width =lua_tonumber(L.get(), -1);
      lua_pop(L.get(), 1);

      lua_getfield(L.get(), -1, "Length");
      belt.Length =lua_tonumber(L.get(), -1);
      lua_pop(L.get(), 1);

      lua_getfield(L.get(), -1, "WidthN");
      belt.WidthN = lua_tonumber(L.get(), -1);
      lua_pop(L.get(), 1);

      lua_getfield(L.get(), -1, "Splices");
      belt.Splices =lua_tonumber(L.get(), -1);
      lua_pop(L.get(), 1);
    }

    global_belt_parameters = belt;

    GocatorConstants gocator;

    lua_status = lua_getglobal(L.get(),"gocator");

    if(lua_istable(L.get(),-1)) {
      lua_getfield(L.get(), -1, "Fps");
      gocator.Fps = lua_tonumber(L.get(), -1);
      lua_pop(L.get(), 1);
    }

    constants_gocator = gocator;

    return 0;
  }
    
  void init_config(std::string f) {
    struct sigaction sigIntHandler;

    sigIntHandler.sa_handler = ::sigint_handler;
    sigemptyset(&sigIntHandler.sa_mask);
    sigIntHandler.sa_flags = 0;

    sigaction(SIGINT, &sigIntHandler, NULL);
    
    
    auto json = slurpfile(f);
    lua_transition(f);
		auto config = nlohmann::json::parse(json);
    constants_device = mk_device(config);
    global_profile_parameters = mk_profile_parameters(config);
    global_conveyor_parameters = mk_conveyor_parameters(config);
    global_belt_parameters = mk_belt_parameters(config);
    global_scan_parameters = mk_scan_parameters(config);
    global_webapi = mk_webapi_urls(config);
    global_filters = mk_filters(config);
    sqlite_gocator_config = mk_sqlite_gocator(config);
    dbscan_config = mk_dbscan(config);
    revolution_sensor_config = mk_revolution_sensor(config);
    communications_config = mk_communications(config);
    fiducial_config = mk_fiducial(config);
    config_origin_detection = mk_origin_detection(config);
    global_config = config;

    measurements.init(); // Needs to be last
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