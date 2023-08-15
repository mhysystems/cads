#pragma once

#include <string>
#include <memory>

#include <gocator_reader_base.h>
#include <msg.h>
#include <io.hpp>
#include <filters.h>
#include <coro.hpp>

namespace cads
{

  struct MeasureConfig {
    bool Enable;
  };

  struct ProfileConfig
  {
    double Width;
    double NaNPercentage;
    double ClipHeight;
    IIRFilterConfig IIRFilter;
    long long PulleySamplesExtend;
    RevolutionSensorConfig RevolutionSensor;
    Conveyor conveyor;
    Dbscan dbscan;
    MeasureConfig measureConfig;
  };

  void store_profile_only();
  void cads_remote_main();
  void cads_local_main(std::string);
  bool direct_process();
  void generate_signal();
  void stop_gocator();
  void process_profile(ProfileConfig, Io& gocatorFifo, Io& next);
  void process_identity(Io& gocatorFifo, Io& next);
  cads::coro<cads::msg,cads::msg,1> profile_decimation_coro(double, long long, cads::Io &next);
}
