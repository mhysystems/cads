#pragma once

#include <string>

#include <msg.h>
#include <io.hpp>
#include <filters.h>
#include <coro.hpp>

#include <edge_detection.h>
#include <constants.h>

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
    double PulleyEstimatorInit;
    double ClampToZeroHeight;
    double TypicalSpeed;
    double PulleyCircumference;
    size_t WidthN;
    IIRFilterConfig IIRFilter;
    RevolutionSensorConfig RevolutionSensor;
    MeasureConfig measureConfig;
  };

  void store_profile_only();
  void cads_remote_main();
  void cads_local_main(std::string);
  bool direct_process();
  void generate_signal();
  void stop_gocator();
  void process_profile(ProfileConfig, Io<msg>& gocatorFifo, Io<msg>& next);
  void profile_pulley_translate(ProfileConfig, Io<msg>& gocatorFifo, Io<msg>& next);
  
  void process_identity(Io<msg>& gocatorFifo, Io<msg>& next);

  msg prs_to_scan(msg);
  msg profilePartion_to_scan(msg m);
  std::tuple<std::string,std::string,std::string> caasMsg(std::string sub,std::string head, std::string data);




  /** @brief Decimates the number z samples for the Caas alignment screen.

  @note Subsamples the profile to fit required number of samples.

  @param width_n vector of samples representing a profile
  @param modulo blah
  @return set of pointer pairs pointing into z.

  */

  cads::coro<cads::msg,cads::msg,1> profile_decimation_coro(long long, long long, cads::Io<msg> &next);
  coro<cads::msg,cads::msg,1> file_csv_coro2(std::string filename, ProfileSection section, double x_min);

  std::function<msg(msg)> mk_align_profile(size_t);
  msg partition_profile(msg m, Dbscan dbscan);

}
