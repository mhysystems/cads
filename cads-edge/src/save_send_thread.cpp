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
#include <coms.h>
#include <db.h>
#include <spdlog/spdlog.h>

using namespace moodycamel;
using namespace std::chrono;


namespace {
  cads::coro<int, std::tuple<int, int, cads::profile>, 1> null_profile_coro()
  {

    while (true)
    {
      auto [data, terminate] = co_yield 0;

      if (terminate)
        break;
    }

    co_return 0;
  }

  cads::coro<int, double,1> null_last_y_coro()
  {

    while (true)
    {
      auto [data, terminate] = co_yield 0;

      if (terminate)
        break;
    }

    co_return 0;
  }
}


namespace cads
{
  void save_send_thread(Conveyor conveyor, Belt belt, bool remote_reg, cads::Io<msg> &profile_fifo, cads::Io<msg> &next)
  {
    
    spdlog::get("cads")->debug(R"({{func = '{}', msg = '{}'}})", __func__,"Entering Thread");

    namespace sml = boost::sml;

    auto scan_begin = date::utc_clock::now();
    auto scan_filename_init = fmt::format("scan-{}.sqlite",scan_begin.time_since_epoch().count());
    
    struct global_t
    {
      cads::coro<int, std::tuple<int, int, cads::profile>, 1> store_profile;
      long idx = 0;
      coro<int, double,1> store_last_y;
      coro<int, profile, 1> store_scan;
      GocatorProperties gocator_properties;
      Conveyor conveyor;
      Belt belt;
      date::utc_clock::time_point scan_begin;
      cads::Io<msg> & cps;
      bool remote_reg;

      global_t(cads::Io<msg> &m, Conveyor c, Belt b, date::utc_clock::time_point sb, std::string s, bool rg) : 
        store_profile(rg ? store_profile_coro() : ::null_profile_coro()), 
        store_last_y(rg ? store_last_y_coro() : ::null_last_y_coro()), 
        store_scan(store_scan_coro(s)), 
        conveyor(c),
        belt(b),
        scan_begin(sb),
        cps(m), 
        remote_reg(rg){};

    } global(next,conveyor,belt,scan_begin,scan_filename_init,remote_reg);


    struct transitions
    {

      auto operator()() const noexcept
      {
        using namespace sml;

        const auto store_action = [](global_t &global, const scan_t &e)
        {
          global.store_profile.resume({0, global.idx++, e.value});
          global.store_scan.resume(e.value);
          global.store_last_y.resume(e.value.y);
        };

        const auto complete_belt_action = [](global_t &global, const CompleteBelt &e)
        {
          global.idx = 0;

          auto scan_end = date::utc_clock::now();
          auto scan_filename = fmt::format("scan-{}.sqlite",global.scan_begin.time_since_epoch().count());
          store_scan_gocator(global.gocator_properties,scan_filename);
          store_scan_conveyor(global.conveyor,scan_filename);
          store_scan_belt(global.belt,scan_filename);
          
          auto new_scan_filename = fmt::format("scan-{}.sqlite",scan_end.time_since_epoch().count());
          create_scan_db(new_scan_filename);
          transfer_profiles(scan_filename,new_scan_filename,e.start_value + 1 + e.length); // +1 as sqlite begins at 1 not 0

          // needs to be after transfer_profiles because uploader can run and delete scan before 
          // transfer complete
          cads::state::scan scan = {
            global.scan_begin,
            scan_filename,
            int64_t(e.start_value),
            int64_t(e.length),
            0,
            1,
            global.conveyor.Site,
            global.conveyor.Name,
            global.remote_reg
          };
        
          global.store_scan = store_scan_coro(new_scan_filename);
          global.scan_begin = scan_end;
          update_scan_state(scan);   
          
          cads::state::scan scan2 = {
            global.scan_begin,
            new_scan_filename,
            0,
            0,
            0,
            0,
            global.conveyor.Site,
            global.conveyor.Name,
            global.remote_reg
          };

          store_scan_state(scan2);  


          global.cps.enqueue({msgid::complete_belt, 0});

        };

        const auto reset_globals_action = [](global_t &global)
        {
          global.idx = 0;
          global.store_scan.terminate();
          auto scan_filename = fmt::format("scan-{}.sqlite",global.scan_begin.time_since_epoch().count());
          std::filesystem::remove(scan_filename);
          
          auto scan_end = date::utc_clock::now();
          auto new_scan_filename = fmt::format("scan-{}.sqlite",scan_end.time_since_epoch().count());
          global.store_scan = store_scan_coro(new_scan_filename);
          global.scan_begin = scan_end;

          cads::state::scan scan2 = {
            global.scan_begin,
            new_scan_filename,
            0,
            0,
            0,
            0,
            global.conveyor.Site,
            global.conveyor.Name,
            global.remote_reg
          };

          store_scan_state(scan2);  

        };

        const auto select_data = [](global_t &global,const Select &select)
        {
          auto scan_filename = fmt::format("scan-{}.sqlite",global.scan_begin.time_since_epoch().count());
          auto rows = fetch_scan(select.begin,select.begin+select.size,scan_filename);
          select.fifo->enqueue(rows);
        };

        const auto init_gocator_properties_action = [](global_t &global,const GocatorProperties &v)
        {
          global.gocator_properties = v;
        };


        return make_transition_table(
            *"init"_s + event<GocatorProperties> / init_gocator_properties_action = "invalid_data"_s,
            "invalid_data"_s + event<begin_sequence_t> / reset_globals_action = "valid_data"_s,
            "valid_data"_s + event<scan_t> / store_action = "valid_data"_s,
            "valid_data"_s + event<CompleteBelt> / complete_belt_action = "valid_data"_s,
            "valid_data"_s + event<Select> / select_data = "valid_data"_s,
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
        global.cps.enqueue(m);
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
        sm.process_event(CompleteBelt{get<CompleteBelt>(get<1>(m))});
        global.cps.enqueue(m);
        break;
      case msgid::gocator_properties:
        global.cps.enqueue(m);
        sm.process_event(GocatorProperties{get<GocatorProperties>(get<1>(m))});
        break;
      case msgid::select:
        sm.process_event(get<Select>(get<1>(m)));
        break;
      default:
          global.cps.enqueue(m);
        break;
      }
    }

    global.store_scan.terminate();
    auto scan_filename = fmt::format("scan-{}.sqlite",global.scan_begin.time_since_epoch().count());
    std::filesystem::remove(scan_filename);
    spdlog::get("cads")->debug(R"({{func = '{}', msg = '{}'}})", __func__,"Exiting Thread");
  }
}
