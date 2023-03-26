#include <chrono>
#include <future>
#include <type_traits>

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wstringop-overflow="
#pragma GCC diagnostic ignored "-Wuseless-cast"
#pragma GCC diagnostic ignored "-Wmaybe-uninitialized"
#pragma GCC diagnostic ignored "-Wshadow"

#include <date/date.h>
#include <date/tz.h>
#include <boost/sml.hpp>

#pragma GCC diagnostic pop

#include <save_send_thread.h>
#include <constants.h>
#include <coms.h>
#include <db.h>
#include <spdlog/spdlog.h>

using namespace moodycamel;
using namespace std::chrono;

namespace {

  std::tuple<double,double,double,double> get_gocator_subset(cads::GocatorProperties g) 
  {
    //struct{double yResolution; double xResolution; double zResolution; double zOffset; double m_encoder_resolution; double m_frame_rate;};
    return {std::get<2>(g),std::get<3>(g),global_config["width_n"].get<double>(),std::get<5>(g)};
    //return {g.zResolution,g.zOffset,global_config["width_n"].get<double>(),g.m_frame_rate};
  }

}


namespace cads
{
  void save_send_thread(BlockingReaderWriterQueue<msg> &profile_fifo, BlockingReaderWriterQueue<msg> &next)
  {
    namespace sml = boost::sml;

    auto scan_begin = date::utc_clock::now();
    auto scan_filename_init = fmt::format("scan-{}.sqlite",scan_begin.time_since_epoch().count());
    
    struct global_t
    {
      cads::coro<int, std::tuple<int, int, cads::profile>, 1> store_profile = store_profile_coro();
      long idx = 0;
      coro<int, double,1> store_last_y = store_last_y_coro();
      coro<int, z_type, 1> store_scan;
      GocatorProperties gocator_properties;
      date::utc_clock::time_point scan_begin;
      BlockingReaderWriterQueue<msg> & cps;

      global_t(BlockingReaderWriterQueue<msg> &m, date::utc_clock::time_point sb, std::string s) : store_scan(store_scan_coro(s)), scan_begin(sb), cps(m){};

    } global(next,scan_begin,scan_filename_init);


    struct transitions
    {

      auto operator()() const noexcept
      {
        using namespace sml;

        const auto store_action = [](global_t &global, const scan_t &e)
        {
          global.store_profile.resume({0, global.idx++, e.value});
          global.store_scan.resume(e.value.z);
          global.store_last_y.resume(e.value.y);
        };

        const auto complete_belt_action = [](global_t &global, const complete_belt_t &e)
        {
          global.idx = 0;

          auto scan_end = date::utc_clock::now();
          auto scan_filename = fmt::format("scan-{}.sqlite",global.scan_begin.time_since_epoch().count());
          store_scan_gocator(get_gocator_subset(global.gocator_properties),scan_filename);
          store_scan_properties({global.scan_begin,scan_end},scan_filename);
          
          store_scan_state(scan_filename);          
          global.cps.enqueue({msgid::complete_belt, 0});
          
          auto new_scan_filename = fmt::format("scan-{}.sqlite",scan_end.time_since_epoch().count());
          global.store_scan = store_scan_coro(new_scan_filename);
          global.scan_begin = scan_end;

        };

        const auto reset_globals_action = [](global_t &global)
        {
          global.idx = 0;
          global.store_scan.terminate();
          auto scan_filename = fmt::format("scan-{}.sqlite",global.scan_begin.time_since_epoch().count());
          std::filesystem::remove(scan_filename);
          
          auto scan_end = date::utc_clock::now();
          auto filename = fmt::format("scan-{}.sqlite",std::chrono::system_clock::now().time_since_epoch().count());
          auto new_scan_filename = fmt::format("scan-{}.sqlite",scan_end.time_since_epoch().count());
          global.store_scan = store_scan_coro(new_scan_filename);
          global.scan_begin = scan_end;

        };

        const auto init_gocator_properties_action = [](global_t &global,const GocatorProperties &v)
        {
          global.gocator_properties = v;
        };


        return make_transition_table(
            *"init"_s + event<GocatorProperties> / init_gocator_properties_action = "invalid_data"_s,
            "invalid_data"_s + event<begin_sequence_t> / reset_globals_action = "valid_data"_s,
            "valid_data"_s + event<scan_t> / store_action = "valid_data"_s,
            "valid_data"_s + event<complete_belt_t> / complete_belt_action = "valid_data"_s,
            "valid_data"_s + event<end_sequence_t> / reset_globals_action = "invalid_data"_s);
      }
    };

    cads::msg m;
    sml::sm<transitions> sm{global};

    for (auto loop = true; loop;)
    {
      profile_fifo.wait_dequeue(m);

      switch (get<0>(m))
      {
      case msgid::finished:
        loop = false;
        break;
      case msgid::scan:
        sm.process_event(scan_t{get<profile>(get<1>(m))});
        break;
      case msgid::begin_sequence:
        sm.process_event(begin_sequence_t{});
        break;
      case msgid::end_sequence:
        sm.process_event(end_sequence_t{});
        break;
      case msgid::complete_belt:
        sm.process_event(complete_belt_t{get<double>(get<1>(m))});
        break;
      case msgid::gocator_properties:
        sm.process_event(GocatorProperties{get<GocatorProperties>(get<1>(m))});
        break;
      default:
        break;
      }
    }

    global.store_scan.terminate();
    auto scan_filename = fmt::format("scan-{}.sqlite",global.scan_begin.time_since_epoch().count());
    std::filesystem::remove(scan_filename);
    spdlog::get("cads")->info("Stopping save_send_thread");
  }
}
