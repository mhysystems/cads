#pragma once

#include <string>

namespace cads
{

  void store_profile_only();
  void upload_profile_only(std::string params = "",std::string db_name = "");
  void process();
  bool direct_process();
  void generate_signal();
  void stop_gocator();
  void dump_gocator_log();

}
