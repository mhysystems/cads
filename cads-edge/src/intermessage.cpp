#include <string>

#include <constants.h>
#include <coms.h>

namespace cads {
  
  bool between(constraints::value_type range, double value) {
    return get<0>(range) <= value && value <= get<1>(range);  
  }

  void publish_CurrentLength(double value) {
    auto valid = between(global_constraints.CurrentLength,value);
    publish_meta_realtime("CurrentLength",value, valid);
  }

  void publish_SurfaceSpeed(double value) {
    auto valid = between(global_constraints.SurfaceSpeed,value);
    publish_meta_realtime("SurfaceSpeed",value, valid);
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
    constraints::value_type range {
      get<0>(global_constraints.CurrentLength) / get<0>(global_constraints.SurfaceSpeed)
      ,get<1>(global_constraints.CurrentLength) / get<1>(global_constraints.SurfaceSpeed)
    };

    auto valid = between(range,value);
    publish_meta_realtime("RotationPeriod",value, valid);
  }

}