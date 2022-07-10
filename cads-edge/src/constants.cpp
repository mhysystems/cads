#include <constants.h>
#include <init.h>

nlohmann::json global_config;

namespace cads {
  void init_config(std::string f) {
    auto json = slurpfile(f);
		global_config = nlohmann::json::parse(json);
  }

  void drop_config() {
    global_config = nlohmann::json();
  }

}