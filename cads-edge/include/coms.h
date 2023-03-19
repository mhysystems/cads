#pragma once

#include <string>
#include <vector>
#include <tuple>


#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wuseless-cast"

#include <date/date.h>
#include <date/tz.h>
#include <blockingconcurrentqueue.h>

#pragma GCC diagnostic pop

#include <profile.h>
#include <constants.h>
#include <coro.hpp>

namespace cads
{
  void realtime_publish_thread(bool&);
  void remote_control_thread(moodycamel::BlockingConcurrentQueue<int> &,bool&);
  void publish_meta_realtime(std::string Id, double value, bool valid);
  void http_post_realtime(double y_area, double value);
  std::tuple<std::chrono::time_point<date::utc_clock, std::chrono::seconds>,bool> http_post_whole_belt(int, int, int);
  std::vector<profile> http_get_frame(double y, int len, date::utc_clock::time_point chrono);
  std::tuple<int,bool> remote_addconveyor(Conveyor params); 
  std::tuple<int,bool> remote_addbelt(Belt params); 

  cads::coro<int, cads::profile, 1> post_profiles_coro(cads::meta meta);
}
