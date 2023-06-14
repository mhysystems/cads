#pragma once

#include <io.hpp>

namespace cads
{
   void dynamic_processing_thread(cads::Io &profile_fifo, cads::Io &next_fifo, int width);
}