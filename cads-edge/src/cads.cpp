#include <exception>
#include <string>
#include <vector>
#include <tuple>
#include <thread>
#include <algorithm>
#include <unordered_map>
#include <expected>

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
#include <err.h>

using namespace std;
using namespace moodycamel;
using namespace std::chrono;


namespace 
{
  struct ProfileError : cads::errors::Err
  {
    
    ProfileError(const char* file, const char* func, int line, const Err& child) : 
      Err(file,func,line,child){}
    
    std::shared_ptr<ProfileError> clone() const {
      return std::shared_ptr<ProfileError>(clone_impl());
    }

    ProfileError* clone_impl() const
    {
      return new ProfileError(*this);
    }
  };
}


namespace cads
{
  
msg partition_profile(msg m, Dbscan dbscan)
{
  auto id = std::get<0>(m);
  if(id != msgid::scan)
  {
    return m;
  }
  
  auto profile = std::get<cads::profile>(std::get<1>(m));
  auto partitions = conveyor_profile_detection(profile,dbscan);
  
  return {msgid::profile_partitioned,cads::ProfilePartitioned{.partitions = std::move(partitions), .scan = std::move(profile)}}; 
}


std::function<msg(msg)> 
  mk_align_profile()
  {
    return [=](msg m) mutable -> msg
    {
      auto id = std::get<0>(m);
      if(id == msgid::profile_partitioned)
      {
        auto profile_part = std::get<ProfilePartitioned>(std::get<1>(m));
        auto status = is_alignable(profile_part.partitions);
        if(status)
        {
          if(profile_part.partitions.contains(ProfileSection::Left) && profile_part.partitions.contains(ProfileSection::Right)) { 
            regression_compensate(profile_part.scan.z,linear_regression(extract_pulley_coords(profile_part.scan.z,profile_part.partitions)).gradient);
          }else {
            auto pulley_coords = extract_pulley_coords(profile_part.scan.z,profile_part.partitions);
            auto pulley_size = (double)std::get<0>(pulley_coords).data.size();
            auto pulley_gradient = linear_regression(std::move(pulley_coords)).gradient;
            
            auto belt_coords = extract_belt_coords(profile_part.scan.z,profile_part.partitions);
            auto belt_size = (double)std::get<0>(belt_coords).data.size();
            auto belt_gradient = linear_regression(std::move(belt_coords)).gradient;

            auto gradient = pulley_gradient*(pulley_size / (pulley_size + belt_size)) + belt_gradient*(belt_size / (pulley_size + belt_size));

            regression_compensate(profile_part.scan.z,gradient);
          }
          return {id,profile_part};
        }else {
          return {msgid::error,::ProfileError(__FILE__,__func__,__LINE__,status).clone()}; 
        }

      }else{
        return m;
      }
    };
  }

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

  coro<cads::msg,cads::msg,1> profile_decimation_coro(long long widthn, long long modulo, cads::Io<msg> &next)
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
            
            auto nan_ratio = nan_cnt / p.z.size();
            auto pwidth = p.z.size() * gp.xResolution;

            spdlog::get("cads")->debug(R"({{where = '{}', id = '{}', value = [{},{},{}], msg = 'values are [nan_cnt,size,nan_ratio]'}})", 
            __func__,
            "gocator caas profile",
            nan_cnt,p.z.size(),nan_ratio);

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

  void process_identity(Io<msg>& gocatorFifo, Io<msg>& next) {
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

  void process_profile(ProfileConfig config, Io<msg>& gocatorFifo, Io<msg>& next)
  {
    spdlog::get("cads")->debug(R"({{func = '{}', msg = '{}'}})", __func__,"Entering Thread");

    const auto x_width = config.Width;
    const auto nan_percentage = config.NaNPercentage;
    const auto width_n = (int)config.WidthN; // gcc-11 on ubuntu cannot compile doubles in ranges expression
    const auto clip_height = (z_element)config.ClipHeight;
    const auto clamp_zero = (z_element)config.ClampToZeroHeight;
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

    auto iirfilter_pulley_level = mk_iirfilterSoS(config.IIRFilter.sos);
    auto iirfilter_left_edge = mk_iirfilterSoS(config.IIRFilter.sos);
    auto iirfilter_right_edge = mk_iirfilterSoS(config.IIRFilter.sos);

    auto delay = mk_delay(config.IIRFilter.delay);

    int64_t cnt = 0;

    auto dc_filter = mk_dc_filter();
    auto pulley_speed = mk_pulley_stats(config.TypicalSpeed,config.PulleyCircumference);

    long drop_profiles = config.IIRFilter.skip; // Allow for iir fillter too stablize

    auto pulley_estimator = mk_pulleyfitter(z_resolution, config.PulleyEstimatorInit);

    auto pulley_rev = config.RevolutionSensor.source == RevolutionSensorConfig::Source::length ? mk_pseudo_revolution(config.PulleyCircumference) : mk_pulley_revolution(config.RevolutionSensor);

    do
    {

      gocatorFifo.wait_dequeue(m);
      auto m_id = get<0>(m);

      if (m_id != cads::msgid::profile_partitioned)
      {
        next.enqueue(m);
        break;
      }

      ++cnt;

      
      auto profile_partitioned = get<ProfilePartitioned>(get<1>(m));
      auto p = profile_partitioned.scan;

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

      auto gg = extract_pulley_coords(p.z,profile_partitioned.partitions);
      auto pulley_level = pulley_estimator(std::get<0>(extract_pulley_coords(p.z,profile_partitioned.partitions)).data);

      auto left_edge = profile_partitioned.partitions.contains(ProfileSection::Belt) ? 
        std::get<0>(profile_partitioned.partitions[ProfileSection::Belt])
        : size_t(0);
      
      auto right_edge = profile_partitioned.partitions.contains(ProfileSection::Belt) ? 
        std::get<1>(profile_partitioned.partitions[ProfileSection::Belt])
        : iz.size();

      auto pulley_filtered = (z_element)iirfilter_pulley_level(pulley_level);
      auto left_edge_filtered = (z_element)iirfilter_left_edge(left_edge);
      auto right_edge_filtered = (z_element)iirfilter_right_edge(right_edge);

      // Required for iirfilter stabilisation
      if (drop_profiles > 0)
      {
        --drop_profiles;
        continue;
      }

      if(right_edge_filtered > iz.size()) {
        spdlog::get("cads")->error("{} > z.size()", "right_edge_filtered",right_edge_filtered);
        right_edge_filtered = (z_element)iz.size(); 
      } 
      
      if(left_edge_filtered > iz.size()) {
        spdlog::get("cads")->error("{} > z.size()", "left_edge_filtered",left_edge_filtered);
        continue;
      } 

      if(right_edge_filtered < 0) {
        spdlog::get("cads")->error("{}({}) < 0 )", "right_edge_filtered",right_edge_filtered);
        continue;
      } 
      
      if(left_edge_filtered < 0) {
        spdlog::get("cads")->error("{}({}) < 0 ", "left_edge_filtered",left_edge_filtered);
        left_edge_filtered = 0; 
      } 

      if(left_edge_filtered >= right_edge_filtered )
      {
        spdlog::get("cads")->error("left_edge_filtered({}) >= right_edge_filtered({})", left_edge_filtered, right_edge_filtered);
        continue;
      }

      if(config.measureConfig.Enable) {
        next.enqueue({msgid::measure,Measure::MeasureMsg{"pulleylevel",0,date::utc_clock::now(),pulley_filtered}});
        next.enqueue({msgid::measure,Measure::MeasureMsg{"beltedgeposition",0,date::utc_clock::now(),double(ix + left_edge_filtered * x_resolution)}});
      }

      auto pulley_level_unbias = dc_filter(pulley_filtered);

      auto [delayed, dd] = delay({{p.time,iy,ix,iz}, (int)left_edge_filtered, (int)left_edge, (int)right_edge, (int)right_edge_filtered});

      if (!delayed)
        continue;

      auto [delayed_profile, left_edge_index_avg, left_edge_index, right_edge_index, right_edge_index_avg] = dd;
      auto y = delayed_profile.y;
      auto x = delayed_profile.x_off;
      auto z = delayed_profile.z;

      PulleyRevolution ps;
      if (config.RevolutionSensor.source == RevolutionSensorConfig::Source::height_raw)
      {
        ps = pulley_rev(pulley_filtered);
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

      pulley_level_compensate(z, -pulley_filtered, clip_height, clamp_zero);
      interpolation_linear(z.begin()+left_edge_index, z.begin()+right_edge_index, [](float a){ return std::isnan(a) || a == 0;});
      
      auto interpolated = interpolate_to_widthn({z.begin()+left_edge_index_avg,z.begin()+right_edge_index_avg},width_n);

      next.enqueue({msgid::pulley_revolution_scan,PulleyRevolutionScan{std::get<0>(ps),std::get<1>(ps), cads::profile{delayed_profile.time,y, x + left_edge_index_avg * x_resolution, std::move(interpolated)}}});

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
      spdlog::get("cads")->error("Luascript doesn't contain coroutine namned mainco");
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
