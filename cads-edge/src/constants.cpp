#include <constants.h>
#include <init.h>

nlohmann::json global_config;



namespace cads {  
  constraints global_constraints;
  profile_parameters global_profile_parameters;
  
  auto mk_contraints(nlohmann::json config) {
    using namespace std;

    auto current_length = config["constraints"]["current_length"].get<constraints::value_type>();
    auto surface_speed = config["constraints"]["surface_speed"].get<constraints::value_type>();
    auto pulley_oscillation = config["constraints"]["pulley_oscillation"].get<constraints::value_type>();
    auto cads_to_origin = config["constraints"]["cads_to_origin"].get<constraints::value_type>();
    auto rotation_period = config["constraints"]["rotation_period"].get<constraints::value_type>();
    auto barrel_height = config["constraints"]["barrel_height"].get<constraints::value_type>();

    return constraints{current_length,surface_speed,pulley_oscillation,cads_to_origin,rotation_period,barrel_height};

  }

  auto mk_profile_parameters(nlohmann::json config) {
    using namespace std;
    auto left_edge_nan = global_config["left_edge_nan"].get<int>();
    auto right_edge_nan = global_config["right_edge_nan"].get<int>();
    auto spike_filter = global_config["spike_filter"].get<int>();
    auto sobel_filter = global_config["sobel_filter"].get<int>();

    return profile_parameters{left_edge_nan,right_edge_nan,spike_filter,sobel_filter};

  }
  
  void init_config(std::string f) {
    auto json = slurpfile(f);
		global_config = nlohmann::json::parse(json);
    global_constraints = mk_contraints(global_config);
    global_profile_parameters = mk_profile_parameters(global_config);
  }

  void drop_config() {
    global_config = nlohmann::json();
  }



}