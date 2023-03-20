#pragma once 

#include <variant>
#include <tuple>
#include <string>
#include <profile.h>

namespace cads
{ 
  
  enum msgid{resolutions,scan,finished,begin_sequence,end_sequence,complete_belt};
  using resolutions_t = std::tuple<double,double,double,double,double,double>;
  using scan_t = struct {cads::profile value;};
  using begin_sequence_t = struct {};
  using end_sequence_t = struct {};
  using complete_belt_t = struct {double value;};
  using msg = std::tuple<msgid,std::variant<resolutions_t,cads::profile,std::string,long,double>>;
}