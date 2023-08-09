#pragma once

#include <io.hpp>

namespace cads
{
  struct DynamicProcessingConfig
  {
    long long WidthN;
  };

  void dynamic_processing_thread(DynamicProcessingConfig, cads::Io &profile_fifo, cads::Io &next_fifo);
}