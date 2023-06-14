#pragma once 

#include <variant>
#include <tuple>
#include <string>
#include <profile.h>

namespace cads
{ 
  using PulleyRevolutionScan = std::tuple<bool, double, cads::profile>;
  enum msgid{gocator_properties,scan,finished,begin_sequence,end_sequence,complete_belt, pulley_revolution_scan, nothing};
  using GocatorProperties = std::tuple<double,double,double,double,double,double>;
  //using GocatorProperties = struct{double yResolution; double xResolution; double zResolution; double zOffset; double m_encoder_resolution; double m_frame_rate;};
  using scan_t = struct {cads::profile value;};
  using begin_sequence_t = struct {};
  using end_sequence_t = struct {};
  using complete_belt_t = struct {double value;};
  using msg = std::tuple<msgid,std::variant<cads::GocatorProperties,cads::profile,std::string,long,double,cads::PulleyRevolutionScan>>;
}