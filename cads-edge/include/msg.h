#pragma once 

#include <variant>
#include <tuple>
#include <string>
#include <profile.h>

namespace cads
{
  enum msgid{resolutions,scan,finished,lua};
  using resolutions_t = std::tuple<double,double,double,double,double>;
  using msg = std::tuple<msgid,std::variant<resolutions_t,cads::profile,std::string,int>>;
}