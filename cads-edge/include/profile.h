#pragma once

#include <vector>
#include <tuple>
#include <unordered_map>

#include <profile_t.h>

namespace cads
{
  enum class ProfileSection {Left,Belt,Right};
  using ConveyorProfile = std::unordered_map<ProfileSection,std::tuple<size_t,size_t>>;

  bool operator==(const profile &, const profile &);

  
  struct transient
  {
    double y;
  };

  std::tuple<z_element, z_element> find_minmax_z(const profile &ps);

  z_type trim_nan(const z_type &z);

  double average(const z_type &);

  z_type profile_decimate(z_type z, size_t width);

}