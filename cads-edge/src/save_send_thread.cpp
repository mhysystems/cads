#include <chrono>
#include <future>

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wstringop-overflow="
#pragma GCC diagnostic ignored "-Wuseless-cast"
#pragma GCC diagnostic ignored "-Wmaybe-uninitialized"
#pragma GCC diagnostic ignored "-Wshadow"

#include <date/date.h>
#include <date/tz.h>
#include <fmt/core.h>
#include <fmt/chrono.h>
#include <spdlog/spdlog.h>
#include <boost/sml.hpp>

#pragma GCC diagnostic pop

#include <save_send_thread.h>
#include <constants.h>
#include <coms.h>
#include <db.h>

using namespace moodycamel;
using namespace std::chrono;

namespace cads
{

  coro<long, std::tuple<long,double>, 1> daily_upload_coro(long revid)
  {

    using namespace date;
    using namespace std;

    hours trigger_hour;
    auto sts = global_config["daily_start_time"].get<std::string>();
    auto drop_uploads = global_config["drop_uploads"].get<int>();

    if (sts != "now"s)
    {
      std::stringstream st{sts};

      system_clock::duration run_in;
      st >> parse("%R", run_in);

      trigger_hour = chrono::floor<chrono::hours>(run_in);
    }
    else
    {
      auto now = current_zone()->to_local(system_clock::now());
      auto today = chrono::floor<chrono::days>(now);
      auto daily_time = duration_cast<seconds>(now - today);
      trigger_hour = chrono::floor<chrono::hours>(daily_time);
    }

    std::chrono::time_point<date::local_t, std::chrono::days> today;
    std::future<date::utc_clock::time_point> fut;
    bool terminate = false;
    long idx = 0;
    double belt_length = 0;
    std::tuple<long,double> args;

    enum state_t
    {
      pre_upload,
      uploading,
      post_upload
    };

    auto state = pre_upload;

    for (; !terminate;)
    {
      std::tie(args, terminate) = co_yield revid;
      std::tie(idx,belt_length) = args;

      if (terminate)
        continue;

      switch (state)
      {
      case pre_upload:
      {
        auto now = current_zone()->to_local(system_clock::now());
        today = chrono::floor<chrono::days>(now);
        auto daily_time = duration_cast<seconds>(now - today);
        auto current_hour = chrono::floor<chrono::hours>(daily_time);

        if (current_hour >= trigger_hour)
        {
          if (drop_uploads == 0)
          {
            fut = std::async(http_post_whole_belt, revid++, idx, belt_length);
            state = uploading;
            spdlog::get("cads")->info("Posting a belt");
          }
          else
          {
            --drop_uploads;
            spdlog::get("cads")->info("Dropped upload. Drops remaining:{}", drop_uploads);
          }
        }
        break;
      }

      case uploading:
        if (fut.wait_for(std::chrono::seconds(0)) == std::future_status::ready)
        {
          fut.get();
          revid--;
          state = post_upload;
        }
        break;

      case post_upload:
      {
        auto now = current_zone()->to_local(system_clock::now());
        if (today != chrono::floor<chrono::days>(now))
        {
          spdlog::get("cads")->info("Switch to waiting");
          state = pre_upload;
        }
        break;
      }
      }
    }
  }

  void save_send_thread(BlockingReaderWriterQueue<msg> &profile_fifo)
  {
    namespace sml = boost::sml;

    struct global_t
    {
      cads::coro<int, std::tuple<int, int, cads::profile>, 1> store_profile = store_profile_coro();
      coro<long, std::tuple<long,double>, 1> daily_upload = daily_upload_coro(0);
      long sequence_cnt = 0;
      long revid = 0;
      long idx = 0;
      double belt_length = 0;
    } global;

    struct transitions
    {

      auto operator()() const noexcept
      {
        using namespace sml;

        const auto store_action = [](global_t &global, const scan_t &e)
        {
          if (e.value.y == 0)
          {
            if (global.sequence_cnt++ > 0)
            {
              bool terminate = false;
              std::tie(global.revid, terminate) = global.daily_upload.resume({global.idx,global.belt_length});
            }
            global.idx = 0;
          }

          global.store_profile.resume({global.revid, global.idx++, e.value});
        };

        const auto update_belt_length = [](global_t &global, const belt_length_t &e)
        {
          global.belt_length = e.value;
        };

        const auto invalid_action = [](global_t &global)
        {
          global.sequence_cnt = 0;
          global.idx = 0;
          global.revid = 0;
        };

        return make_transition_table(
            *"invalid_data"_s + event<begin_sequence_t> = "valid_data"_s, 
            "valid_data"_s + event<scan_t> / store_action = "valid_data"_s, 
            "valid_data"_s + event<belt_length_t> / update_belt_length = "valid_data"_s, 
            "valid_data"_s + event<end_sequence_t> / invalid_action = "invalid_data"_s);
      }
    };

    cads::msg m;
    sml::sm<transitions> sm{global};
    auto start = std::chrono::high_resolution_clock::now();
    int64_t cnt = 0;

    for (auto loop = true; loop;)
    {
      profile_fifo.wait_dequeue(m);

      switch (get<0>(m))
      {
      case msgid::scan:
        sm.process_event(scan_t{get<profile>(get<1>(m))});
        break;
      case msgid::begin_sequence:
        sm.process_event(begin_sequence_t{});
        break;
      case msgid::end_sequence:
        sm.process_event(end_sequence_t{});
        break;
      case msgid::belt_length:
        sm.process_event(belt_length_t{get<double>(get<1>(m))});
        break;
      default:
        loop = false;
        continue;
      }
    }

    global.store_profile.terminate();
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
    spdlog::get("cads")->info("DB PROCESSING - CNT: {}, DUR: {}, RATE(ms):{} ", cnt, duration, cnt / duration);

    // spdlog::get("cads")->info("Final Upload");
    // http_post_whole_belt(revid, idx); // For replay and not having a complete belt, so something is uploaded
    spdlog::get("cads")->info("Stopping save_send_thread");
  }
}