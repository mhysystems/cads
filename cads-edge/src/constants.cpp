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

  auto mk_communications(nlohmann::json config) {
    using namespace std;
    
    auto NatsUrl = config["communications"]["NatsUrl"].get<std::string>();
    auto UploadRows = config["communications"]["UploadRows"].get<size_t>();

    return cads::Communications{NatsUrl,UploadRows};

  }

  auto mk_device(nlohmann::json config) {
    
    auto serial = config["Device"]["Serial"].get<long>();

    return cads::Device{serial};
  }

  auto mk_webapi_urls(nlohmann::json config) {
    
    auto add_scan = config["add_scan"].get<cads::webapi_urls::value_type>();

    return cads::webapi_urls{add_scan};

  }

  auto mk_upload(nlohmann::json config) {
    
    auto period = config["upload"]["Period"].get<long>();
    auto webapi = mk_webapi_urls(config["upload"]["webapi"]);

    return cads::UploadConfig{std::chrono::seconds(period),webapi};
  }

  auto mk_databases(nlohmann::json config) {
    auto profile_db_name = config["databases"]["profile_db_name"].get<std::string>();
    auto state_db_name = config["databases"]["state_db_name"].get<std::string>();
    auto transient_db_name = config["databases"]["transient_db_name"].get<std::string>();

    return cads::Databases{profile_db_name,state_db_name,transient_db_name};
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
  Communications communications_config;
  UploadConfig upload_config;
  HeartBeat constants_heartbeat;
  Databases database_names;
  std::optional<std::string> luascript_name(std::nullopt);

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
    communications_config = mk_communications(config);
    upload_config = mk_upload(config);
    database_names = mk_databases(config);
    constants_heartbeat = mk_heartbeat(config);

    if(config.contains("luascript") && !config["luascript"].get<std::string>().empty()) {
      luascript_name = config["luascript"].get<std::string>();
    }

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
  
    params_json["Site"] = Site;
    params_json["Name"] = Name;
    params_json["Timezone"] = Timezone;
    params_json["PulleyCircumference"] = PulleyCircumference;
    params_json["TypicalSpeed"] = TypicalSpeed;

    return params_json.dump();
  }

  Belt::operator std::string() const {
    
    nlohmann::json params_json;
  
    params_json["Serial"] =  Serial;
    params_json["PulleyCover"] = PulleyCover;
    params_json["CordDiameter"] = CordDiameter;
    params_json["TopCover"] = TopCover;
    params_json["Length"] = Length;
    params_json["Width"] = Width;
    params_json["WidthN"] = WidthN;
    
    return params_json.dump();
  }

}