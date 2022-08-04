#pragma once
#include <profile.h>
#include <vector>
#include <functional>


namespace cads
{
  void spike_filter(z_type& z, int window_size);
  void nan_filter(z_type& z);
  void barrel_height_compensate(z_type& z, z_element z_off, z_element z_max);
  std::function<double(double)> mk_iirfilterSoS();
  std::function<std::tuple<bool,std::tuple<y_type,double,z_type>>(std::tuple<y_type,double,z_type>)> mk_delay(size_t len);


} // namespace cads
