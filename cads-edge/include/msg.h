#pragma once 

#include <variant>
#include <tuple>
#include <string>
#include <profile.h>

namespace cads
{ 
  
  enum msgid{resolutions,scan,finished,barrel_rotation_cnt,begin_sequence,end_sequence};
  using resolutions_t = std::tuple<double,double,double,double,double>;
  using scan_t = struct {cads::profile value;};
  using begin_sequence_t = struct {long value;};
  using end_sequence_t = struct {long value;};
  using msg = std::tuple<msgid,std::variant<resolutions_t,cads::profile,std::string,long>>;
}