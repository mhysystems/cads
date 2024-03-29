#pragma once

#include <vector>
#include <tuple>

#include <profile_t.h>
#include <edge_detection.h>
#include <vec_nan_free.h>
#include <err.h>

namespace cads
{

  bool operator==(const profile &, const profile &);

  
  struct transient
  {
    double y;
  };

  std::tuple<z_element, z_element> find_minmax_z(const profile &ps);

  z_type trim_nan(const z_type &z);

  double average(const z_type &);

  z_type profile_decimate(z_type z, size_t width);
  ProfilePartitions conveyor_profile_detection(const profile &, Dbscan config);

  z_type extract_partition(const ProfilePartitioned&, ProfileSection);

  std::tuple<vector_NaN_free,vector_NaN_free> 
  extract_pulley_coords(const z_type &z, ProfilePartitions conveyor);
  
  std::tuple<vector_NaN_free,vector_NaN_free> 
  extract_belt_coords(const z_type &z, ProfilePartitions conveyor);
  
  errors::ErrCode is_alignable(const ProfilePartitions &part);

  profile packzbits(profile,double);
  profile unpackzbits(profile,double);

}