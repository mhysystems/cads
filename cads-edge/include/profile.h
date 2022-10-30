#pragma once

#include <vector>
#include <tuple>
#include <cstdint>
#include <deque>
#include <limits>
#include <functional>

namespace cads
{
  using z_element = float; //int16_t;
  using y_type = double;
  using z_type = std::vector<z_element>;
  
  struct profile{y_type y; double x_off; z_type z;}; 
  using profile_window = std::deque<profile>;
  bool operator==(const profile&, const profile&);

  const cads::profile null_profile{std::numeric_limits<cads::y_type>::max(),std::numeric_limits<double>::max(),{}};

  struct profile_params{double y_res; double x_res; double z_res; double z_off; double encoder_res; double z_max;};

  bool compare_samples(const profile& a, const profile& b, int threshold); 
  std::tuple<z_element,z_element> find_minmax_z(const profile& ps);
  std::tuple<z_element,z_element,bool>  barrel_offset(const z_type& win,double z_height_mm);
  std::function<int(int,int)> mk_edge_adjust(int left_edge_index_previous, int width_n);
  std::tuple<z_type,z_type> partition_profile(const z_type& z,int,int);
  double barrel_mean(const z_type& z,int,int);
  std::tuple<double,double> pulley_left_right_mean(const z_type& z, int left_edge_index, int right_edge_index);
  
}