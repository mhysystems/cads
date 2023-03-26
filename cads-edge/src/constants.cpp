#include <tuple>
#include <algorithm> 
#include <sstream>

#include <date/tz.h>

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

  auto mk_contraints(nlohmann::json config) {
    using namespace std;

    auto current_length = config["constraints"]["current_length"].get<cads::constraints::value_type>();
    auto surface_speed = config["constraints"]["surface_speed"].get<cads::constraints::value_type>();
    auto pulley_oscillation = config["constraints"]["pulley_oscillation"].get<cads::constraints::value_type>();
    auto cads_to_origin = config["constraints"]["cads_to_origin"].get<cads::constraints::value_type>();
    auto rotation_period = config["constraints"]["rotation_period"].get<cads::constraints::value_type>();
    auto barrel_height = config["constraints"]["barrel_height"].get<cads::constraints::value_type>();
    auto z_unbiased = config["constraints"]["z_unbiased"].get<cads::constraints::value_type>();

    return cads::constraints{current_length,surface_speed,pulley_oscillation,cads_to_origin,rotation_period,barrel_height,z_unbiased};

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
    auto Splices = config["belt"]["Splices"].get<int64_t>();
    int64_t Conveyor = 0;

    return cads::Belt{Id,Installed,PulleyCover,CordDiameter,TopCover,Length,Width,Splices,Conveyor};

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

    return cads::Filters{SchmittThreshold};

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
      source = cads::RevolutionSensor::Source::raw;
    }else {
      source = cads::RevolutionSensor::Source::filtered;  
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
}

namespace cads {  

  constraints global_constraints;
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

  
  void init_config(std::string f) {
    auto json = slurpfile(f);
		auto config = nlohmann::json::parse(json);
    global_constraints = mk_contraints(config);
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
    global_config = config;
  }

  void drop_config() {
    global_config = nlohmann::json();
  }

  bool between(constraints::value_type range, double value) {
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