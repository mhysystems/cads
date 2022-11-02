#pragma once
#include <profile.h>
#include <vector>
#include <functional>
#include <constants.h>


namespace cads
{
  void spike_filter(z_type& z, int window_size = global_profile_parameters.spike_filter);
  void nan_filter(z_type& z);
  void barrel_height_compensate(z_type& z, z_element z_off, z_element z_max);
  std::function<double(double)> mk_iirfilterSoS();
  std::function<std::tuple<bool,std::tuple<y_type,double,z_type,int,int>>(std::tuple<y_type,double,z_type,int,int>)> mk_delay(size_t len);
  std::function<cads::z_element(cads::z_element)> mk_schmitt_trigger(cads::z_element ref);
  std::function<cads::z_element(cads::z_element,bool)> mk_amplitude_extraction();


} // namespace cads
