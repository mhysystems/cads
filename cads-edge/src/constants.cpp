#include <tuple>

#include <constants.h>
#include <init.h>

nlohmann::json global_config;

namespace {
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
    using namespace std;
    auto site = config["conveyor"]["site"].get<string>();
    auto name = config["conveyor"]["name"].get<string>();
    auto pulley_cover = config["conveyor"]["pulley_cover"].get<double>();
    auto cord_diameter = config["conveyor"]["cord_diameter"].get<double>();
    auto top_cover = config["conveyor"]["top_cover"].get<double>();
    auto id = config["conveyor"]["id"].get<int>();

    return cads::conveyor_parameters{site,name,pulley_cover,cord_diameter,top_cover,id};

  }

  auto mk_webapi(nlohmann::json config) {
    using namespace std;
    
    auto trim_backslashes = []() {

    };
    
    auto base_url = config["webapi"]["base_rul"].get<string>();
    auto add_conveyor = config["webapi"]["add_conveyor"].get<string>();

    return cads::webapi{base_url,add_conveyor};

  }
}

namespace cads {  

  constraints global_constraints;
  profile_parameters global_profile_parameters;
  conveyor_parameters global_conveyor_parameters;
  webapi global_webapi;
  
  void init_config(std::string f) {
    auto json = slurpfile(f);
		auto config = nlohmann::json::parse(json);
    global_constraints = mk_contraints(config);
    global_profile_parameters = mk_profile_parameters(config);
    global_conveyor_parameters = mk_conveyor_parameters(config);
    global_webapi = mk_webapi(config);
    global_config = config;
  }

  void drop_config() {
    global_config = nlohmann::json();
  }

  bool between(constraints::value_type range, double value) {
    return get<0>(range) <= value && value <= get<1>(range);  
  }


}