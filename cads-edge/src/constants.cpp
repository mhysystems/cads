#include <constants.h>
#include <init.h>

nlohmann::json global_config;



namespace cads {  
  constraints global_constraints;
  
  auto mk_contraints(nlohmann::json config) {
    using namespace std;

    auto current_length = config["constraints"]["current_length"].get<constraints::value_type>();
    auto surface_speed = config["constraints"]["surface_speed"].get<constraints::value_type>();
    auto pulley_oscillation = config["constraints"]["pulley_oscillation"].get<constraints::value_type>();
    auto cads_to_origin = config["constraints"]["cads_to_origin"].get<constraints::value_type>();

    return constraints{current_length,surface_speed,pulley_oscillation,cads_to_origin};

  }
  
  void init_config(std::string f) {
    auto json = slurpfile(f);
		global_config = nlohmann::json::parse(json);
    global_constraints = mk_contraints(global_config);
  }

  void drop_config() {
    global_config = nlohmann::json();
  }



}