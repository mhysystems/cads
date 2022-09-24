#pragma once

#include <string>
#include <vector>


#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wuseless-cast"

#include <date/date.h>
#include <date/tz.h>

#pragma GCC diagnostic pop

#include <profile.h>

namespace cads
{
  void realtime_publish_thread();
  void http_post_meta_realtime(std::string Id, double value);
  void http_post_realtime(double y_area, double value);
  date::utc_clock::time_point http_post_whole_belt(int, int);
  std::vector<profile> http_get_frame(double y, int len, date::utc_clock::time_point chrono);
}
