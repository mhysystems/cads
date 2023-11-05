#pragma once

#include <io.hpp>
#include <constants.h>

namespace cads
{
  void save_send_thread(cads::Conveyor, bool, cads::Io<msg> &, cads::Io<msg>& );
}