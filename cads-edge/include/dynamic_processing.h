#pragma once

#include <coro.hpp>
#include <profile.h>

namespace cads
{
  coro<int, profile> lua_processing_coro();
}