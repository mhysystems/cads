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
#include <db.h>
#include <msg.h>

namespace cads
{
  void remote_control_thread( bool &, moodycamel::BlockingConcurrentQueue<remote_msg> &);
  cads::coro<cads::remote_msg,bool> remote_control_coro();
  std::tuple<state::scan,bool> post_scan(state::scan scan);
  std::vector<profile> http_get_frame(double y, int len, date::utc_clock::time_point chrono);
  cads::coro<int, std::tuple<std::string, std::string, std::string>, 1>  realtime_metrics_coro();
}
