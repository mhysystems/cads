#pragma once

#include <string>
#include <memory>

#include <gocator_reader_base.h>
#include <msg.h>
#include <io.hpp>

namespace cads
{

  struct ProfileConfig
  {
    double Width;
    long long WidthN;
    double NaNPercentage;
    double ClipHeight;
  };

  void store_profile_only();
  void cads_remote_main();
  void cads_local_main(std::string);
  bool direct_process();
  void generate_signal();
  void stop_gocator();
  void process_profile(ProfileConfig, Io& gocatorFifo, Io& next);
  void process_identity(Io& gocatorFifo, Io& next);
}
