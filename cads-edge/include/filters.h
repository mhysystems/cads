#pragma once
#include <profile.h>
#include <vector>
#include <functional>
#include <constants.h>


namespace cads
{
  void spike_filter(z_type& z, int window_size = global_profile_parameters.spike_filter);
  void pulley_level_compensate(z_type& z, z_element z_off, z_element z_max);
  void constraint_clipping(z_type& z, z_element z_off, z_element z_max);
  std::function<double(double)> mk_iirfilterSoS();
  std::function<std::tuple<bool, std::tuple<y_type, double, z_type,int,int,int,z_type>>(std::tuple<y_type, double, z_type,int,int,int,z_type>)> mk_delay(size_t len);
  std::function<cads::z_element(cads::z_element)> mk_schmitt_trigger(const cads::z_element ref, const cads::z_element bias);
  std::function<cads::z_element(cads::z_element)> mk_schmitt_trigger(const cads::z_element bias = 0.0);
  std::function<cads::z_element(cads::z_element,bool)> mk_amplitude_extraction();
  std::function<double(double)> mk_dc_filter();
  std::function<void(z_type &z)> mk_pulley_damp();

} // namespace cads
