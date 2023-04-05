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

namespace cads
{
  using z_element = float; // int16_t;
  using y_type = double;
  using z_type = std::vector<z_element>;
  using zzrange = std::ranges::subrange<z_type::iterator>;
  
  using zrange = std::tuple<z_type::const_iterator, z_type::const_iterator>;
  using z_cluster = std::vector<zrange>;
  using z_clusters = std::vector<z_cluster>;

  struct profile
  {
    y_type y;
    double x_off;
    z_type z;
  };
  using profile_window = std::deque<profile>;
  bool operator==(const profile &, const profile &);

  const cads::profile null_profile{std::numeric_limits<cads::y_type>::max(), std::numeric_limits<double>::max(), {}};

  struct profile_params
  {
    double y_res;
    double x_res;
    double z_res;
    double z_off;
    double encoder_res;
    double z_max;
  };

  struct meta
  {
    std::string site = "";
    std::string conveyor = "";
    std::string chrono = "";
    double x_res = 0;
    double y_res = 0;
    double z_res = 0;
    double z_off = 0;
    double z_max = 0;
    double z_min = 0;
    double Ymax = 0;
    double YmaxN = 0;
    double WidthN = 0;
    long Belt = 0;
  };

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
  
  std::string ClusterErrorToString(ClusterError error);
  std::tuple<double, double, size_t, size_t, z_clusters, ClusterError> pulley_levels_clustered(const z_type &z, std::function<double(const z_type &)> estimator = average);

}