#pragma once

#include <coro.hpp>
#include <msg.h>

namespace cads
{
  coro<double, msg> lua_processing_coro(int width);
}