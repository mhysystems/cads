#pragma once

#include <io.hpp>
#include <constants.h>
#include <scandb.h>

namespace cads
{

  struct ScanStorageConfig
  {
    Belt belt;
    Conveyor conveyor;
    ScanMeta meta;
    bool register_upload;
  };

  void save_send_thread(ScanStorageConfig, cads::Io<msg> &, cads::Io<msg>& );
}