#pragma once

#include <io.hpp>

namespace cads
{
  void save_send_thread(cads::Io &profile_fifo, cads::Io& next);
}