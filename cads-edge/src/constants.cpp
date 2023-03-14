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

    return cads::SqliteGocatorConfig{current_length,fps,forever};
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
    auto Site = config["conveyor"]["Site"].get<std::string>();
    auto Name = config["conveyor"]["Name"].get<std::string>();
    auto Timezone =  date::current_zone()->name();
    
    if(config["conveyor"]["Timezone"].get<std::string>() != "use_os")
    {
      Timezone = config["conveyor"]["Timezone"].get<std::string>();
    }

    auto PulleyCircumference = config["belt"]["PulleyCircumference"].get<double>();
    int64_t Belt = 0;

    return cads::Conveyor{Id,Site,Name,Timezone,PulleyCircumference,Belt};

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
    auto Splices = config["belt"]["Splices"].get<double>();
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
    params_json["Width"] = Length;
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