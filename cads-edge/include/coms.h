#pragma once

#include <string>
#include <vector>

#include <date/date.h>
#include <date/tz.h>

#include <profile.h>

namespace cads
{
  date::utc_clock::time_point http_post_whole_belt(int, int);
  std::vector<profile> http_get_frame(double y, int len, date::utc_clock::time_point chrono);
}
