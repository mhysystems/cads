#pragma once

#include <io.hpp>

namespace cads
{
  struct DynamicProcessingConfig
  {
    long long WidthN;
    long long WindowSize;
    double InitValue;
    std::string LuaCode;
    std::string Entry;
  };

  void dynamic_processing_thread(DynamicProcessingConfig, cads::Io<msg> &profile_fifo, cads::Io<msg> &next_fifo);
}