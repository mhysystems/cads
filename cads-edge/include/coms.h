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

#include <profile_t.h>
#include <constants.h>
#include <coro.hpp>
#include <db.h>
#include <msg.h>

namespace cads
{
  void remote_control_thread(moodycamel::BlockingConcurrentQueue<remote_msg> &, std::atomic<bool> &);
  cads::coro<cads::remote_msg,bool> remote_control_coro(std::atomic<bool>&);
  std::tuple<state::scan,bool> post_scan(state::scan scan,webapi_urls,std::atomic<bool> &terminate) ;
  cads::coro<int, std::tuple<std::string, std::string, std::string>, 1>  realtime_metrics_coro();
  void realtime_metrics_thread(moodycamel::BlockingConcurrentQueue<std::tuple<std::string, std::string, std::string>> &queue, HeartBeat beat, std::atomic<bool> &terminate);
  std::string profile_as_flatbufferstring(profile p, GocatorProperties, double);
}
