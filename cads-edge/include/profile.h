#pragma once

#include <vector>
#include <tuple>
#include <cstdint>
#include <deque>
#include <limits>
#include <functional>
#include <chrono>
#include <string>
#include <ranges>
#include <constants.h>

namespace cads
{
  using z_element = float; // int16_t;
  using y_type = double;
  using z_type = std::vector<z_element>;
  using zrange = std::ranges::subrange<z_type::iterator>;

  using z_cluster = std::vector<zrange>;
  using z_clusters = std::vector<z_cluster>;

  struct profile
  {
    std::chrono::time_point<std::chrono::system_clock> time;
    y_type y;
    double x_off;
    z_type z;
  };
  using profile_window = std::deque<profile>;
  bool operator==(const profile &, const profile &);

  const cads::profile null_profile{ std::chrono::time_point<std::chrono::system_clock>::min(),std::numeric_limits<cads::y_type>::max(), std::numeric_limits<double>::max(), {}};

  struct transient
  {
    double y;
  };

  std::tuple<z_element, z_element> find_minmax_z(const profile &ps);

  z_type trim_nan(const z_type &z);
  void constraint_substitute(z_type &z, z_element z_min, z_element z_max, z_element sub = std::numeric_limits<z_element>::quiet_NaN());
  void recontruct_z(z_type &z, const z_clusters &group);
  double average(const z_type &);
  enum class ClusterError
  {
    None,
    NoClusters,
    ExcessiveClusters,
    ExcessiveNeigbours
  };
  
  z_type decimate(z_type z, double stride);
  std::string ClusterErrorToString(ClusterError error);
  std::tuple<double, double, size_t, size_t, ClusterError> pulley_levels_clustered(z_type &z, Dbscan, std::function<double(const z_type &)> estimator = average);

}