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

  coro<long, std::tuple<long, double>, 1> daily_upload_coro(long read_revid)
  {
    using namespace date;
    using namespace std;

    hours trigger_hour;
    auto sts = global_config["daily_start_time"].get<std::string>();
    auto drop_uploads = global_config["drop_uploads"].get<int>();
    auto daily_upload = global_config["daily_upload"].get<bool>();
    auto write_revid = read_revid;

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
    std::future<std::invoke_result_t<decltype(http_post_whole_belt), int, int, double>> fut;
    bool terminate = false;
    long idx = 0;
    double belt_length = 0;
    std::tuple<long, double> args;

    enum state_t
    {
      pre_upload,
      uploading,
      post_upload,
      no_shedule
    };

    auto state = daily_upload ? pre_upload : no_shedule;

    for (; !terminate;)
    {
      std::tie(args, terminate) =  co_yield write_revid;
      std::tie(idx, belt_length) = args;

      if (terminate)
        continue;

      for (auto process = true; process;)
      {

        switch (state)
        {
        case no_shedule:
        {
          fut = std::async(http_post_whole_belt, read_revid, idx, belt_length);
          write_revid++;
          spdlog::get("cads")->info("Posting a belt. Writing data to revid: {}", write_revid);
          state = uploading;
          break;
        }
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
              fut = std::async(http_post_whole_belt, read_revid, idx, belt_length);
              write_revid++;
              state = uploading;
              spdlog::get("cads")->info("Posting a belt. Writing data to revid: {}. Reading data from revid: {}", write_revid, read_revid);
            }
            else
            {
              --drop_uploads;
              state = post_upload;
              spdlog::get("cads")->info("Dropped upload. Drops remaining:{}", drop_uploads);
            }
          }
          else
          {
            process = false;
          }
          break;
        }
        case uploading:
        {
          if (fut.wait_for(std::chrono::seconds(0)) == std::future_status::ready)
          {
            auto [time, err] = fut.get();
            read_revid = write_revid;
            if (err)
            {
              write_revid++;
              state = daily_upload ? pre_upload : no_shedule;
            }
            else
            {
              write_revid--;
              state = daily_upload ? post_upload : no_shedule;
            }
            spdlog::get("cads")->info("Belt upload thread finished. Writing data to revid: {}", write_revid);
          }
          else
          {
            process = false;
          }
          break;
        }
        case post_upload:
        {
          auto tmp = date::current_zone();//->to_local(std::chrono::system_clock::now());
          auto now = tmp->to_local(std::chrono::system_clock::now());
          auto huh  = chrono::floor<chrono::days>(now);
          if (today != huh)
          {
            state = pre_upload;
          }
          else
          {
            process = false;
          }
          break;
        }
        }
      }    
    }

  }

  void save_send_thread(BlockingReaderWriterQueue<msg> &profile_fifo)
  {
    namespace sml = boost::sml;

    spdlog::get("cads")->info("save_send_thread started");

    struct global_t
    {
      cads::coro<int, std::tuple<int, int, cads::profile>, 1> store_profile = store_profile_coro();
      coro<long, std::tuple<long, double>, 1> daily_upload = daily_upload_coro(0);
      long revid = 0;
      long idx = 0;
    } global;

    struct transitions
    {

      auto operator()() const noexcept
      {
        using namespace sml;

        const auto store_action = [](global_t &global, const scan_t &e)
        {
          auto [co_end, s_err] = global.store_profile.resume({global.revid, global.idx, e.value});
          if (s_err == 101)
          {
            global.idx++;
          }
          else
          {
            // error
          }
        };

        const auto complete_belt_action = [](global_t &global, const complete_belt_t &e)
        {
          bool terminate = false;
          std::tie(terminate, global.revid) = global.daily_upload.resume({global.idx, e.value});
          global.idx = 0;
        };

        const auto reset_globals_action = [](global_t &global)
        {
          global.idx = 0;
          global.revid = 0;
        };

        return make_transition_table(
            *"invalid_data"_s + event<begin_sequence_t> / reset_globals_action = "valid_data"_s,
            "valid_data"_s + event<scan_t> / store_action = "valid_data"_s,
            "valid_data"_s + event<complete_belt_t> / complete_belt_action = "valid_data"_s,
            "valid_data"_s + event<end_sequence_t> / reset_globals_action = "invalid_data"_s);
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
      case msgid::complete_belt:
        sm.process_event(complete_belt_t{get<double>(get<1>(m))});
        break;
      default:
        loop = false;
        continue;
      }
    }

    global.store_profile.terminate();
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
    
    auto rate = duration != 0 ? (double)cnt / duration : 0;
    spdlog::get("cads")->info("DB PROCESSING - CNT: {}, DUR: {}, RATE(ms):{} ", cnt, duration, rate);

    // spdlog::get("cads")->info("Final Upload");
    // http_post_whole_belt(revid, idx); // For replay and not having a complete belt, so something is uploaded
    spdlog::get("cads")->info("Stopping save_send_thread");
  }
}
