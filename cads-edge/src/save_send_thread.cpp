#include <chrono>
#include <future>
#include <type_traits>
#include <algorithm>

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
#include <scandb.h>

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
  void save_send_thread(ScanStorageConfig config, cads::Io<msg> &profile_fifo, cads::Io<msg> &next)
  {
    
    spdlog::get("cads")->debug(R"({{func = '{}', msg = '{}'}})", __func__,"Entering Thread");

    namespace sml = boost::sml;

    auto init_scan_time = date::utc_clock::now();
    auto scan_filename_init = fmt::format("scan-{}.sqlite",init_scan_time.time_since_epoch().count());
    
    struct global_t
    {
      ScanStorageConfig config;
      cads::coro<int, std::tuple<int, int, cads::profile>, 1> store_profile;
      coro<int, double,1> store_last_y;
      coro<int, profile, 1> store_scan;
      date::utc_clock::time_point scan_begin;
      cads::Io<msg> & cps;
      GocatorProperties gocator_properties;
      std::vector<cads::z_element> z_minmax;
      std::vector<double> x_minmax;
      long idx = 0;

      global_t(ScanStorageConfig c, cads::Io<msg> &m, date::utc_clock::time_point sb, std::string s) : 
        config(c),
        store_profile(c.register_upload ? store_profile_coro() : ::null_profile_coro()), 
        store_last_y(c.register_upload ? store_last_y_coro() : ::null_last_y_coro()), 
        store_scan(store_scan_coro(s)), 
        scan_begin(sb),
        cps(m){};

    } global(config
        ,next
        ,init_scan_time
        ,scan_filename_init);


    struct transitions
    {

      auto operator()() const noexcept
      {
        using namespace sml;

        const auto store_action = [](global_t &global, const scan_t &e)
        {
          const auto [z_min,z_max] = std::minmax_element(e.value.z.begin(),e.value.z.end());
          const auto x_min = e.value.x_off;
          const auto x_max = e.value.x_off + e.value.z.size() * global.gocator_properties.xResolution;
          global.z_minmax.push_back(*z_min);
          global.z_minmax.push_back(*z_max);
          global.x_minmax.push_back(x_min);
          global.x_minmax.push_back(x_max);
          global.store_profile.resume({0, global.idx++, e.value});
          if(global.config.meta.ZEncoding == 1) {
            global.store_scan.resume(packzbits(e.value));
          }else{
            global.store_scan.resume(e.value);
          }
          global.store_last_y.resume(e.value.y);
        };

        const auto complete_belt_action = [](global_t &global, const CompleteBelt &e)
        {
          global.idx = 0;
          const auto [z_min,z_max] = std::minmax_element(global.z_minmax.begin() + 2*e.start_value,global.z_minmax.begin()+2*(e.start_value + e.length));
          const auto [x_min,x_max] = std::minmax_element(global.x_minmax.begin() + 2*e.start_value,global.x_minmax.begin()+2*(e.start_value + e.length));

          auto scan_end = date::utc_clock::now();
          auto scan_filename = fmt::format("scan-{}.sqlite",global.scan_begin.time_since_epoch().count());
          store_scan_gocator(global.gocator_properties,scan_filename);
          store_scan_conveyor(global.config.conveyor,scan_filename);
          store_scan_belt(global.config.belt,scan_filename);
          store_scan_limits({.ZMin = *z_min, .ZMax = *z_max, .XMin = *x_min, .XMax = *x_max},scan_filename);
          store_scan_meta(global.config.meta,scan_filename);
          
          // Clear after dereferencing
          global.z_minmax.clear();
          global.x_minmax.clear();
          
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
            global.config.conveyor.Site,
            global.config.conveyor.Name,
            global.config.register_upload
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
            global.config.conveyor.Site,
            global.config.conveyor.Name,
            global.config.register_upload
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
            global.config.conveyor.Site,
            global.config.conveyor.Name,
            global.config.register_upload
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
