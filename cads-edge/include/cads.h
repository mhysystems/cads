#pragma once

#include <string>
#include <memory>

#include <gocator_reader_base.h>
#include <msg.h>
#include <io.hpp>

namespace cads
{

  void store_profile_only();
  void cads_remote_main();
  void cads_local_main(std::string);
  bool direct_process();
  void generate_signal();
  void stop_gocator();
  void process_profile(Io& gocatorFifo, Io& next);
  void process_identity(Io& gocatorFifo, Io& next);
}
