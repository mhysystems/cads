#pragma once

#include <string>
#include <memory>

#include <gocator_reader_base.h>
#include <msg.h>
#include <io.hpp>

namespace cads
{

  void store_profile_only();
  bool direct_process();
  void generate_signal();
  void stop_gocator();
  void dump_gocator_log();
  void process_profile(Io& gocatorFifo, Io& next);
  void process_identity(Io& gocatorFifo, Io& next);
  std::unique_ptr<GocatorReaderBase> mk_gocator(Io &gocatorFifo);
}
