#pragma once
#include <profile.h>
#include <vector>
#include <functional>
#include <constants.h>


namespace cads
{
  void spike_filter(z_type& z, int window_size = global_profile_parameters.spike_filter);
  void barrel_height_compensate(z_type& z, z_element z_off, z_element z_max);
  void constraint_clipping(z_type& z, z_element z_off, z_element z_max);
  std::function<double(double)> mk_iirfilterSoS();
  std::function<std::tuple<bool,std::tuple<y_type,double,z_type,int,int,z_type>>(std::tuple<y_type,double,z_type,int,int,z_type>)> mk_delay(size_t len);
  std::function<cads::z_element(cads::z_element)> mk_schmitt_trigger(cads::z_element ref);
  std::function<cads::z_element(cads::z_element)> mk_schmitt_trigger();
  std::function<cads::z_element(cads::z_element,bool)> mk_amplitude_extraction();
  std::function<double(double)> mk_dc_filter();

} // namespace cads
