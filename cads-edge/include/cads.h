#pragma once

#include <string>
#include <memory>
#include <readerwriterqueue.h>
#include <gocator_reader_base.h>
#include <msg.h>

namespace cads
{

  void store_profile_only();
  void upload_profile_only(std::string params = "",std::string db_name = "");
  void process();
  bool direct_process();
  void generate_signal();
  void stop_gocator();
  void dump_gocator_log();
  std::unique_ptr<GocatorReaderBase> mk_gocator(moodycamel::BlockingReaderWriterQueue<msg> &gocatorFifo, bool trim = true, bool use_encoder = false);
}
