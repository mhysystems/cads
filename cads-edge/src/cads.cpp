#include <exception>
#include <string>
#include <vector>
#include <tuple>
#include <thread>
#include <algorithm>

#include <spdlog/spdlog.h>
#include <lua_script.h>

#include <cads.h>
#include <regression.h>
#include <coms.h>
#include <constants.h>
#include <gocator_reader.h>
#include <edge_detection.h>
#include <coro.hpp>
#include <belt.h>
#include <sampling.h>
#include <upload.h>
#include <utils.hpp>



using namespace std;
using namespace moodycamel;
using namespace std::chrono;

namespace cads
{

msg prs_to_scan(msg m)
{
  auto id = std::get<0>(m);
  if(id == msgid::pulley_revolution_scan)
  {
    auto prs = std::get<PulleyRevolutionScan>(std::get<1>(m));
    return {msgid::scan,std::get<2>(prs)};
  }else{
    return m;
  }
}

coro<cads::msg,cads::msg,1> partition_belt_coro(Dbscan dbscan, cads::Io &next)
  {
    cads::msg empty;
    auto pulley_estimator = mk_pulleyfitter(1.0);    

    for(long cnt = 0;;cnt++){
      
      auto [msg,terminate] = co_yield empty;  
      
      if(terminate) break;

      switch(std::get<0>(msg)) {
          case cads::msgid::gocator_properties: {
          auto p = std::get<GocatorProperties>(std::get<1>(msg));
          pulley_estimator = mk_pulleyfitter(p.zResolution);
        }
        break;
        
        case msgid::scan: {
          auto p = std::get<profile>(std::get<1>(msg));
          auto [pulley_level, pulley_right, ll, lr, cerror] = pulley_levels_clustered(p.z, dbscan,pulley_estimator);    
          //next.enqueue({msgid::caas_msg,cads::CaasMsg{"profile",profile_as_flatbufferstring(p,z_resolution,z_offset)}});

        }
        break;
        default:
          next.enqueue(msg);
      }
    }
  }


  coro<cads::msg,cads::msg,1> profile_decimation_coro(long long widthn, long long modulo, cads::Io &next)
  {
    cads::msg empty;
    GocatorProperties gp;

    for(long cnt = 0;;cnt++){
      
      auto [msg,terminate] = co_yield empty;  
      
      if(terminate) break;

      switch(std::get<0>(msg)) {
        
        case cads::msgid::gocator_properties: {
          auto p = std::get<GocatorProperties>(std::get<1>(msg));
          gp = p;
          spdlog::get("cads")->debug(R"({{where = '{}', id = '{}', value = [{},{},{},{},{},{},{}], msg = 'values are [xResolution,zOffset,zResolution,x,width,z,height]'}})", 
            __func__,
            "gocator properties",
            p.xResolution,p.zOffset,p.zResolution,p.xOrigin,p.width,p.zOrigin,p.height);
        }
        break;
        case msgid::scan: {
          if(cnt % modulo == 0){
            auto p = std::get<profile>(std::get<1>(msg));
            double nan_cnt = std::count_if(p.z.begin(), p.z.end(), [](z_element z)
                  { return std::isnan(z); });
            
            spdlog::get("cads")->debug(R"({{func = '{}', msg = 'number of profile samples {}, mid value {}'}})", __func__,p.z.size(),p.z[size_t(p.z.size()/2)]);
            spdlog::get("cads")->debug(R"({{func = '{}', msg = 'nan count is {}, ratio {}'}})", __func__,nan_cnt, nan_cnt / p.z.size());
            
            auto nan_ratio = nan_cnt / p.z.size();
            
            auto pwidth = p.z.size() * gp.xResolution;

            if(p.z.size() > (size_t)widthn) {
              p.z = profile_decimate(p.z,widthn);
            }

            auto send_gp = gp;
            send_gp.xResolution = pwidth / p.z.size();
            next.enqueue({msgid::caas_msg,cads::CaasMsg{"profile",profile_as_flatbufferstring(p,send_gp,nan_ratio)}});

          }
        }
        break;
        default:
          next.enqueue(msg);
      }
    }
  }

  void process_identity(Io& gocatorFifo, Io& next) {
    msg m;
    do
    {

      gocatorFifo.wait_dequeue(m);
      auto m_id = get<0>(m);

      if(m_id == cads::msgid::gocator_properties) continue;

      if (m_id != cads::msgid::scan)
      {
        break;
      }


      auto p = get<profile>(get<1>(m));
      next.enqueue(m);

    } while (std::get<0>(m) != msgid::finished);
  }

  void process_profile(ProfileConfig config, Io& gocatorFifo, Io& next)
  {
    spdlog::get("cads")->debug(R"({{func = '{}', msg = '{}'}})", __func__,"Entering Thread");

    const auto x_width = config.Width;
    const auto nan_percentage = config.NaNPercentage;
    const auto width_n = (int)config.conveyor.WidthN; // gcc-11 on ubuntu cannot compile doubles in ranges expression
    const auto clip_height = (z_element)config.ClipHeight;
    double x_resolution = 1.0;
    double z_resolution = 1.0;

    cads::msg m;


    gocatorFifo.wait_dequeue(m);
    auto m_id = get<0>(m);

    if (m_id != cads::msgid::gocator_properties)
    {
      std::throw_with_nested(std::runtime_error("preprocessing:First message must be gocator_properties"));
    }
    else
    {
      auto p = get<GocatorProperties>(get<1>(m));
      x_resolution = p.xResolution;
      z_resolution = p.zResolution;
    }

    next.enqueue(m);

    auto iirfilter_left = mk_iirfilterSoS(config.IIRFilter.sos);
    auto iirfilter_right = mk_iirfilterSoS(config.IIRFilter.sos);
    auto iirfilter_left_edge = mk_iirfilterSoS(config.IIRFilter.sos);

    auto delay = mk_delay(config.IIRFilter.delay);

    int64_t cnt = 0;

    auto dc_filter = mk_dc_filter();
    auto pulley_speed = mk_pulley_stats(config.conveyor.TypicalSpeed,config.conveyor.PulleyCircumference);

    long drop_profiles = config.IIRFilter.skip; // Allow for iir fillter too stablize

    auto pulley_estimator = mk_pulleyfitter(z_resolution, config.PulleyEstimatorInit);
    auto belt_estimator = mk_pulleyfitter(z_resolution, 0.0);

    auto filter_window_len = config.PulleySamplesExtend;

    auto pulley_rev = config.RevolutionSensor.source == RevolutionSensorConfig::Source::length ? mk_pseudo_revolution(config.conveyor.PulleyCircumference) : mk_pulley_revolution(config.RevolutionSensor);

    do
    {

      gocatorFifo.wait_dequeue(m);
      auto m_id = get<0>(m);

      if (m_id != cads::msgid::scan)
      {
        next.enqueue(m);
        break;
      }

      ++cnt;

      auto p = get<profile>(get<1>(m));

      if (p.z.size() * x_resolution < x_width * 0.75)
      {
        spdlog::get("cads")->error("Gocator sending profiles with widths less than 0.75 of expected width");
        next.enqueue({msgid::finished,0});
        break;
      }

      if (p.z.size() < (size_t)width_n)
      {
        spdlog::get("cads")->error("Gocator sending profiles with sample number {} less than {}", p.z.size(), width_n);
        next.enqueue({msgid::finished,0});
        break;

      }

      double nan_cnt = std::count_if(p.z.begin(), p.z.end(), [](z_element z)
                                     { return std::isnan(z); });

      if(config.measureConfig.Enable) {
        next.enqueue({msgid::measure,Measure::MeasureMsg{"nancount",0,date::utc_clock::now(),nan_cnt}});
      }

      if ((nan_cnt / p.z.size()) > nan_percentage)
      {
        spdlog::get("cads")->error("Percentage of nan({}) in profile > {}%", nan_cnt, nan_percentage * 100);
        next.enqueue({msgid::finished,0});
        break;
      }

      auto iy = p.y;
      auto ix = p.x_off;
      auto iz = p.z;

      auto [pulley_level, pulley_right, ll, lr, cerror] = pulley_levels_clustered(iz, config.dbscan,pulley_estimator);

      if (cerror != ClusterError::None)
      {
        spdlog::get("cads")->debug("Clustering error : {}", ClusterErrorToString(cerror));
        // store_errored_profile(p.z,ClusterErrorToString(cerror));
      }

      iz.insert(iz.begin(), filter_window_len, (float)pulley_level);
      iz.insert(iz.end(), filter_window_len, (float)pulley_right);
      ll += filter_window_len;
      lr += filter_window_len;

      std::fill(iz.begin(), iz.begin() + ll, pulley_level);
      std::fill(iz.begin() + lr, iz.end(), pulley_right);

      auto z_nan_removed = iz | views::filter([](float a){ return !std::isnan(a); });
      auto bb = select_if(iz,interpolate_to_widthn({z_nan_removed.begin(),z_nan_removed.end()},iz.size()),[](float a){ return std::isnan(a); });
      iz = bb;

      auto pulley_level_filtered = (z_element)iirfilter_left(pulley_level);
      auto pulley_right_filtered = (z_element)iirfilter_right(pulley_right);
      auto left_edge_filtered = (z_element)iirfilter_left_edge(ll);

      // Required for iirfilter stabilisation
      if (drop_profiles > 0)
      {
        --drop_profiles;
        continue;
      }

      if(config.measureConfig.Enable) {
        next.enqueue({msgid::measure,Measure::MeasureMsg{"pulleylevel",0,date::utc_clock::now(),pulley_level_filtered}});
        next.enqueue({msgid::measure,Measure::MeasureMsg{"beltedgeposition",0,date::utc_clock::now(),double(ix + left_edge_filtered * x_resolution)}});
      }

      auto pulley_level_unbias = dc_filter(pulley_level_filtered);

      auto [delayed, dd] = delay({{p.time,iy,ix,iz}, (int)left_edge_filtered, (int)ll, (int)lr, p.z});

      if (!delayed)
        continue;

       auto [delayed_profile, left_edge_index_avg, left_edge_index, right_edge_index, raw_z] = dd;
      auto y = delayed_profile.y;
      auto x = delayed_profile.x_off;
      auto z = delayed_profile.z;

      auto gradient = (pulley_right_filtered - pulley_level_filtered) / (double)z.size();
      regression_compensate(z, 0, z.size(), gradient);

      PulleyRevolution ps;
      if (config.RevolutionSensor.source == RevolutionSensorConfig::Source::height_raw)
      {
        ps = pulley_rev(pulley_level);
      }
      else if(config.RevolutionSensor.source == RevolutionSensorConfig::Source::length) 
      {
        ps = pulley_rev(y);
      }
      else
      {
        ps = pulley_rev(pulley_level_unbias);
      }

      auto [speed, frequency, amplitude] = pulley_speed(ps, pulley_level_unbias, delayed_profile.time);

      if (speed == 0)
      {
        spdlog::get("cads")->error("Belt proabaly stopped.");
        next.enqueue({msgid::stopped,0});
        break;
      }
      
      if(config.measureConfig.Enable) {
        next.enqueue({msgid::measure,Measure::MeasureMsg{"pulleyspeed",0,date::utc_clock::now(),speed}});
        next.enqueue({msgid::measure,Measure::MeasureMsg{"pulleyoscillation",0,date::utc_clock::now(),amplitude}});
      }

      pulley_level_compensate(z, -pulley_level_filtered, clip_height);
      
      auto left_edge_index_aligned = left_edge_index_avg;
      auto right_edge_index_aligned = right_edge_index;

      if (z.size() < size_t(left_edge_index_aligned + width_n))
      {
        // spdlog::get("cads")->debug("Belt width({})[la - {}, l- {}, r - {}] less than required. Filled with zeros",z.size(),left_edge_index_aligned,left_edge_index,right_edge_index);
        // store_errored_profile(raw_z);
        const auto cnt = left_edge_index_aligned + width_n - z.size();
        z.insert(z.end(), cnt, 0.0);
      }
      
      auto z_zero_removed = z | views::filter([](float a){ return a != 0;});
      auto f = select_if({z.begin()+left_edge_index_aligned,z.begin()+left_edge_index_aligned+width_n},interpolate_to_widthn({z_zero_removed.begin(),z_zero_removed.end()},width_n),[](float a){ return a == 0;});

      next.enqueue({msgid::pulley_revolution_scan,PulleyRevolutionScan{std::get<0>(ps),std::get<1>(ps), cads::profile{delayed_profile.time,y, x + left_edge_index_aligned * x_resolution, {f.begin(), f.end()}}}});

    } while (std::get<0>(m) != msgid::finished);


    spdlog::get("cads")->debug(R"({{func = '{}', msg = '{}'}})", __func__,"Exiting Thread");

  }

  std::tuple<std::string,std::string,std::string> caasMsg(std::string sub,std::string head, std::string data)
  {
    auto caas_sub = "caas."s + std::to_string(constants_device.Serial) + "."s + sub;
    return std::make_tuple(caas_sub,head,data);
  }

  void cads_local_main(std::string f) 
  {
    namespace fs = std::filesystem;

    fs::path luafile{f};
    luafile.replace_extension("lua");
    
    std::atomic<bool> terminate_upload = false;
    std::atomic<bool> terminate_metrics = false;
    moodycamel::BlockingConcurrentQueue<std::tuple<std::string, std::string, std::string>> queue;
    std::jthread save_send(upload_scan_thread, std::ref(terminate_upload), upload_config);
    std::jthread realtime_metrics(realtime_metrics_thread, std::ref(queue), constants_heartbeat, std::ref(terminate_metrics));
  
    auto [L,err] = run_lua_config(f);

    if(err) {
      return;
    }

    auto lua_type = lua_getglobal(L.get(),"mainco");
    
    if(lua_type != LUA_TTHREAD) {
      spdlog::get("cads")->error("TODO");
      return;
    }
    
    auto mainco = lua_tothread(L.get(), -1); 

    for(auto first = true;!terminate_signal;first=false)
    {
      
      if(first) {
        push_externalmsg(mainco,&queue);
      }else{
        lua_pushboolean(mainco,false);
      }
      
      auto nargs = 0;
      auto mainco_status = lua_resume(mainco,L.get(),1,&nargs);
      
      if(mainco_status == LUA_YIELD) 
      {
      
      }
      else if(mainco_status == LUA_OK) // finished 
      {
        break;
      } 
      else
      {
        spdlog::get("cads")->error("{}: lua_resume: {}",__func__,lua_tostring(mainco,-1));
        break;
      }

    }

    auto mainco_status = lua_status(mainco);
    
    if( mainco_status == LUA_OK || mainco_status == LUA_YIELD ) {
      auto nargs = 0;
      lua_pushboolean(mainco,true);
      lua_resume(mainco,L.get(),1,&nargs);
    }

    terminate_upload = true; // stops upload thread
    terminate_metrics = true; // stops metrics thread
  }

  void cads_remote_main() {
    
    std::atomic<bool> terminate_upload = false;
    std::atomic<bool> terminate_metrics = false;
    std::atomic<bool> terminate_remote = false;
    moodycamel::BlockingConcurrentQueue<std::tuple<std::string, std::string, std::string>> queue;
    moodycamel::BlockingConcurrentQueue<remote_msg> remote_queue;
    std::jthread save_send(upload_scan_thread, std::ref(terminate_upload),upload_config);
    std::jthread realtime_metrics(realtime_metrics_thread, std::ref(queue), constants_heartbeat, std::ref(terminate_metrics));
    std::jthread remote_control(remote_control_thread, std::ref(remote_queue), std::ref(terminate_remote));
   
    while(!terminate_signal)
    {
      try {
        GocatorReader::LaserOff();
        remote_msg rmsg;
            
        // Wait for start message
        while(!terminate_signal) {
          remote_queue.wait_dequeue(rmsg);
          
          if(rmsg.index() == 0) break;

        }

        if(terminate_signal) continue;

        auto start_msg = std::get<Start>(rmsg);
        auto [L,err] = run_lua_code(start_msg.lua_code);

        if(err) {
          continue;
        }

        auto lua_type = lua_getglobal(L.get(),"mainco");
        
        if(lua_type != LUA_TTHREAD) {
          spdlog::get("cads")->error("TODO");
          continue;
        }

        
        auto mainco = lua_tothread(L.get(), -1); 

        for(auto first = true;!terminate_signal;first=false)
        {
          
          if(first) {
            push_externalmsg(mainco,&queue);
          }else{
            lua_pushboolean(mainco,false);
          }
          
          auto nargs = 0;
          auto mainco_status = lua_resume(mainco,L.get(),1,&nargs);
          
          if(mainco_status == LUA_YIELD) 
          {
          
          }
          else if(mainco_status == LUA_OK) // finished 
          {
            break;
          } 
          else
          {
            spdlog::get("cads")->error("{}: lua_resume: {}",__func__,lua_tostring(mainco,-1));
            break;
          }


          auto have_msg = remote_queue.try_dequeue(rmsg);
          
          if(have_msg && rmsg.index() != 2) break;


        }
        auto mainco_status = lua_status(mainco);
        
        if(mainco_status == LUA_YIELD ) {
          auto nargs = 0;
          lua_pushboolean(mainco,true);
          lua_resume(mainco,L.get(),1,&nargs);
        }

        queue.enqueue(caasMsg("scancomplete","",""));
      }catch(std::exception& ex) {
        spdlog::get("cads")->error(R"({{func = '{}', msg = '{}'}})", __func__,ex.what());
        queue.enqueue(caasMsg("error","",ex.what()));
      }
    }
  
    queue.enqueue(caasMsg("scancomplete","",""));
    terminate_upload = true; // stops upload thread
    terminate_metrics = true;
    terminate_remote = true;
  }


  void store_profile_only()
  {
#if 0
    auto terminate_subscribe = false;
    moodycamel::BlockingConcurrentQueue<int> nats_queue;
    Adapt<BlockingReaderWriterQueue<msg>> gocatorFifo{BlockingReaderWriterQueue<msg>(4096 * 1024)};
    std::jthread remote_control(remote_control_thread, std::ref(nats_queue), std::ref(terminate_subscribe));

    auto gocator  = mk_gocator(gocatorFifo);

    gocator->Start();

    cads::msg m;

    gocatorFifo.wait_dequeue(m);

    if (get<0>(m) != cads::msgid::gocator_properties)
    {
      std::throw_with_nested(std::runtime_error("First message must be resolutions"));
    }
    else
    {
      auto now = chrono::floor<chrono::milliseconds>(date::utc_clock::now());
      auto ts = date::format("%FT%TZ", now);
      spdlog::get("cads")->info("Let's go! - {}", ts);
    }

    auto store_profile = store_profile_coro();

    auto y_max_length = global_belt_parameters.Length * 1.02;
    double Y = 0.0;
    int idx = 0;
    double reset_y = 0.0;

    do
    {

      gocatorFifo.wait_dequeue(m);
      auto m_id = get<0>(m);

      switch (m_id)
      {
      case cads::msgid::scan:
      {
        auto p = get<profile>(get<1>(m));
        Y = p.y;
        p.y -= reset_y;
        int ignore = 0;
        if (nats_queue.try_dequeue(ignore))
        {
          reset_y = p.y;
          p.y = 0;
          auto now = chrono::floor<chrono::milliseconds>(date::utc_clock::now());
          auto ts = date::format("%FT%TZ", now);
          spdlog::get("cads")->info("Y reset! - {}", ts);
        }

        store_profile.resume({0, idx++, p});
        break;
      }
      case cads::msgid::gocator_properties:
      {
        break;
      }

      case cads::msgid::finished:
      {
        break;
      }
      default:
        continue;
      }

    } while (get<0>(m) != msgid::finished && Y < 2 * y_max_length);

    store_profile.terminate();
    terminate_subscribe = true;

    gocator->Stop();
    spdlog::get("cads")->info("Cads raw data dumping finished");
  #endif
  }


  void generate_signal()
  {
#if 0
    Adapt<BlockingReaderWriterQueue<msg>> gocatorFifo{BlockingReaderWriterQueue<msg>(4096 * 1024)};

    auto gocator = mk_gocator(gocatorFifo);
    gocator->Start(984.0); // TODO

    auto iirfilter = mk_iirfilterSoS();
    auto delay = mk_delay(global_config["iirfilter"]["delay"]);

    cads::msg m;

    gocatorFifo.wait_dequeue(m);
    auto m_id = get<0>(m);

    if (m_id == cads::msgid::finished)
    {
      return;
    }

    if (m_id != cads::msgid::gocator_properties)
    {
      std::throw_with_nested(std::runtime_error("preprocessing:First message must be gocator_properties"));
    }

    auto gocator_props = get<GocatorProperties>(get<1>(m));

    auto differentiation = mk_dc_filter();

    std::ofstream filt("filt.txt");

    auto pulley_estimator = mk_pulleyfitter(gocator_props.zResolution, -212.0);

    do
    {

      gocatorFifo.wait_dequeue(m);
      auto m_id = get<0>(m);

      if (m_id == cads::msgid::scan)
      {

        auto p = get<profile>(get<1>(m));

        auto [pulley_left, pulley_right, ll, lr, cerror] = pulley_levels_clustered(p.z, pulley_estimator);

        auto bottom_avg = pulley_left;
        auto bottom_filtered = iirfilter(bottom_avg);

        filt << bottom_avg << "," << bottom_filtered << '\n';
        filt.flush();

        auto [delayed, dd] = delay({p, 0, 0, 0, p.z});
        if (!delayed)
          continue;

        auto [delayed_profile, ignore_d, ignore_a, ignore_b, ignore_c] = dd;
      }

    } while (std::get<0>(m) != msgid::finished);

    gocator->Stop();
    spdlog::get("cads")->info("Gocator Stopped");
  #endif
  }

  
  void stop_gocator()
  {
    GocatorReader::LaserOff();
  }

}
