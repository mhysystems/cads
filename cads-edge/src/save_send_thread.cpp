#include <chrono>
#include <future>

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wstringop-overflow="
#pragma GCC diagnostic ignored "-Wuseless-cast"
#pragma GCC diagnostic ignored "-Wmaybe-uninitialized"

#include <date/date.h>
#include <date/tz.h>
#include <fmt/core.h>
#include <fmt/chrono.h>
#include <spdlog/spdlog.h>

#pragma GCC diagnostic pop

#include <save_send_thread.h>
#include <constants.h>
#include <coms.h>
#include <db.h>

using namespace std;
using namespace moodycamel;
using namespace std::chrono;

constexpr size_t buffer_warning_increment = 4092;

namespace cads
{
  void save_send_thread(BlockingReaderWriterQueue<msg> &profile_fifo)
  {
    using namespace date;
    using namespace chrono;

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

    cads::msg m;

    enum state
    {
      waiting,
      processing,
      waitthread,
      finished
    };

    state s = waiting;
    auto store_profile = store_profile_coro();
    std::chrono::time_point<date::local_t, std::chrono::days> today;
    std::future<date::utc_clock::time_point> fut;
    profile p;
    int revid = 0, idx = 0;
    auto buffer_size_warning = buffer_warning_increment;

    auto start = std::chrono::high_resolution_clock::now();
    int64_t cnt = 0;

    while (true)
    {
      ++cnt;
      profile_fifo.wait_dequeue(m);

      if (get<0>(m) == msgid::scan)
      {
        p = get<profile>(get<1>(m));
      }
      else
      {
        break;
      }

      switch (s)
      {
      case waiting:
      {
        auto now = current_zone()->to_local(system_clock::now());
        today = chrono::floor<chrono::days>(now);
        auto daily_time = duration_cast<seconds>(now - today);
        auto current_hour = chrono::floor<chrono::hours>(daily_time);

        if (p.y == 0 && current_hour >= trigger_hour)
        {
          // store_profile.resume({revid, idx++, p}); REMOVEME
          s = processing;
        }
        break;
      }

      case processing:
      {
        if (p.y == 0)
        {
          if (drop_uploads == 0)
          {
            fut = std::async(http_post_whole_belt, revid++, idx);
            s = waitthread;
            spdlog::get("cads")->info("Finished processing a belt");
          }
          else
          {
            --drop_uploads;
            spdlog::get("cads")->info("Dropped upload. Drops remaining:{}", drop_uploads);
            s = finished;
          }
        }
        else
        {
          // store_profile.resume({revid, idx++, p}); REMOVEME
        }
        break;
      }
      case waitthread:
        if (fut.wait_for(std::chrono::seconds(0)) == std::future_status::ready && p.y == 0.0)
        {
          fut.get();
          revid = 0;
          s = finished;
        }
        break;
      case finished:
      {
        auto now = current_zone()->to_local(system_clock::now());
        if (today != chrono::floor<chrono::days>(now))
        {
          spdlog::get("cads")->info("Switch to waiting");
          s = waiting;
        }
      }
      }

      if (p.y == 0.0)
        idx = 0;

      store_profile.resume({revid, idx++, p});

      if (profile_fifo.size_approx() > buffer_size_warning)
      {
        spdlog::get("cads")->warn("Saving to DB showing signs of not being able to keep up with data source. Size {}", buffer_size_warning);
        buffer_size_warning += buffer_warning_increment;
      }
    }

    store_profile.terminate();
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
    spdlog::get("cads")->info("DB PROCESSING - CNT: {}, DUR: {}, RATE(ms):{} ", cnt, duration, cnt / duration);

    // spdlog::get("cads")->info("Final Upload");
    // http_post_whole_belt(revid, idx); // For replay and not having a complete belt, so something is uploaded
    spdlog::get("cads")->info("Stopping save_send_thread");
  }

}