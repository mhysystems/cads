#include <string>

#include <spdlog/spdlog.h>

#include <constants.h>
#include <coms.h>


namespace cads {
  
  void publish_CurrentLength(double value) {
    auto valid = between(global_constraints.CurrentLength,value);
    publish_meta_realtime("CurrentLength",value, valid);
  }

  void publish_SurfaceSpeed(double value) {
    auto valid = between(global_constraints.SurfaceSpeed,value);
    publish_meta_realtime("SurfaceSpeed",value, valid);
    spdlog::get("cads")->debug("Surface Speed {}",value);
  }

  void publish_PulleyOscillation(double value) {
    auto valid = between(global_constraints.PulleyOcillation,value);
    publish_meta_realtime("PulleyOscillation",value, valid);
  }

  void publish_CadsToOrigin(double value) {
    auto valid = between(global_constraints.CadsToOrigin,value);
    publish_meta_realtime("CadsToOrigin",value, valid);
  }

  void publish_RotationPeriod(double value) {
    auto valid = between(global_constraints.RotationPeriod,value);
    publish_meta_realtime("RotationPeriod",value, valid);
  }
  
}