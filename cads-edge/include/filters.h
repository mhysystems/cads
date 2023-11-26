#pragma once

#include <vector>
#include <functional>

#include <profile_t.h>

namespace cads
{
  struct IIRFilterConfig
  {
    long long skip;
    long long delay;
    std::vector<std::vector<double>> sos;
  };

  void pulley_level_compensate(z_type& z, z_element z_off, z_element z_max);
  std::function<double(double)> mk_iirfilterSoS(std::vector<std::vector<double>>);
  std::function<std::tuple<bool, std::tuple<profile,int,int,int,int>>(std::tuple<profile,int,int,int,int>)> mk_delay(size_t len);
  std::function<cads::z_element(cads::z_element)> mk_schmitt_trigger(const cads::z_element ref, const cads::z_element bias);
  std::function<cads::z_element(cads::z_element,cads::z_element)> mk_schmitt_trigger(const cads::z_element ref);
  std::function<cads::z_element(cads::z_element,bool)> mk_amplitude_extraction();
  std::function<double(double)> mk_dc_filter();

} // namespace cads
