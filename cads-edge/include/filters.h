#pragma once
#include <profile.h>
#include <vector>
#include <functional>
#include <constants.h>


namespace cads
{
  struct IIRFilterConfig
  {
    long long skip;
    long long delay;
    std::vector<std::vector<double>> sos;
  };
  //void spike_filter(z_type& z, int window_size = global_profile_parameters.spike_filter);
  void spike_filter(z_type& z, int window_size = global_profile_parameters.spike_filter, int lead = global_profile_parameters.left_edge_nan);
  void pulley_level_compensate(z_type& z, z_element z_off, z_element z_max);
  std::function<double(double)> mk_iirfilterSoS(std::vector<std::vector<double>>);
  std::function<std::tuple<bool, std::tuple<profile,int,int,int,z_type>>(std::tuple<profile,int,int,int,z_type>)> mk_delay(size_t len);
  std::function<cads::z_element(cads::z_element)> mk_schmitt_trigger(const cads::z_element ref, const cads::z_element bias);
  std::function<cads::z_element(cads::z_element)> mk_schmitt_trigger(const cads::z_element bias = 0.0);
  std::function<cads::z_element(cads::z_element,bool)> mk_amplitude_extraction();
  std::function<double(double)> mk_dc_filter();
  void gaussian(z_type& z);
} // namespace cads
