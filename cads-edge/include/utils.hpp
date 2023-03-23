#pragma once

#include <string>

#include <date/date.h>
#include <date/tz.h>

namespace cads
{
  std::string to_str(date::utc_clock::time_point);
  date::utc_clock::time_point to_clk(std::string);
}