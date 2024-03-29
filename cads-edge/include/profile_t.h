#pragma once

#include <vector>
#include <chrono>
#include <limits>
#include <unordered_map>

namespace cads
{
  using z_element = float;
  using y_type = double;
  using z_type = std::vector<z_element>;

  struct profile
  {
    std::chrono::time_point<std::chrono::system_clock> time;
    y_type y;
    double x_off;
    z_type z;
  };


  using zrange = std::ranges::subrange<cads::z_type::const_iterator>;
  using z_cluster = std::vector<zrange>;

  enum class ProfileSection {Left,Belt,Right,End};
  using ProfilePartitions = std::unordered_map<ProfileSection,std::tuple<size_t,size_t>>;
  struct ProfilePartitioned { ProfilePartitions partitions; profile scan; };  

  const cads::profile null_profile{ std::chrono::time_point<std::chrono::system_clock>::min(),std::numeric_limits<cads::y_type>::max(), std::numeric_limits<double>::max(), {}};  
}