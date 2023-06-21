#include <cads.h>
#include <regression.h>

#include <db.h>
#include <coms.h>
#include <constants.h>
#include <fiducial.h>

#include <gocator_reader.h>
#include <sqlite_gocator_reader.h>

#include <exception>
#include <limits>
#include <string>
#include <vector>
#include <tuple>
#include <algorithm>
#include <thread>
#include <ranges>
#include <chrono>
#include <sstream>
#include <fstream>
#include <iostream>

#include <spdlog/spdlog.h>
#include <core/SCAMP.h>
#include <lua_script.h>

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wuseless-cast"

#include <date/date.h>
#include <date/tz.h>
#include <fmt/core.h>
#include <fmt/chrono.h>

#pragma GCC diagnostic pop

#include <filters.h>
#include <edge_detection.h>
#include <coro.hpp>
#include <dynamic_processing.h>
#include <save_send_thread.h>
#include <origin_detection_thread.h>
#include <belt.h>
#include <interpolation.h>
#include <upload.h>
#include <utils.hpp>



using namespace std;
using namespace moodycamel;
using namespace std::chrono;
using CadsMat = cv::UMat; // cv::cuda::GpuMat

namespace
{
  using namespace cads;

  void upload_conveyorbelt_parameters()
  {

    auto [conveyor_id, err] = fetch_conveyor_id();

    if (!err && conveyor_id == 0)
    {
      auto [new_id, err] = remote_addconveyor(global_conveyor_parameters);
      if (!err)
      {
        store_conveyor_id(new_id);
        conveyor_id = new_id;
      }
    }

    auto [belt_id, errb] = fetch_belt_id();

    if (!errb && belt_id == 0 && conveyor_id != 0)
    {
      auto belt = global_belt_parameters;
      belt.Conveyor = conveyor_id;
      auto [new_id, err] = remote_addbelt(belt);

      if (!err)
      {
        store_belt_id(new_id);
        belt_id = new_id;
      }
    }
  }

#if 0
  Process_Status process_impl(IO<msg> auto &next)
  {

    //std::jthread uploading_conveyorbelt_parameters(upload_conveyorbelt_parameters);

    Adapt<BlockingReaderWriterQueue<msg>> gocatorFifo{BlockingReaderWriterQueue<msg>(4096 * 1024)};
    Adapt<BlockingReaderWriterQueue<msg>> winFifo(BlockingReaderWriterQueue<msg>(4096 * 1024));

    const auto x_width = global_belt_parameters.Width;
    const auto nan_percentage = global_config["nan_%"].get<double>();
    const auto width_n = global_config["width_n"].get<int>();
    const auto clip_height = global_config["clip_height"].get<z_element>();
    const auto use_encoder = global_config["use_encoder"].get<bool>();

    auto gocator = cads::mk_gocator(gocatorFifo, use_encoder);
    gocator->Start();

    cads::msg m;

    gocatorFifo.wait_dequeue(m);
    auto m_id = get<0>(m);

    if (m_id == cads::msgid::finished)
    {
      return Process_Status::Finished;
    }

    if (m_id != cads::msgid::gocator_properties)
    {
      std::throw_with_nested(std::runtime_error("preprocessing:First message must be gocator_properties"));
    }
    else
    {
      auto [y_resolution, x_resolution, z_resolution, z_offset, encoder_resolution, encoder_framerate] = get<GocatorProperties>(get<1>(m));
      spdlog::get("cads")->info("Gocator y_resolution:{}, x_resolution:{}, z_resolution:{}, z_offset:{}, encoder_resolution:{}, encoder_framerate:{}", y_resolution, x_resolution, z_resolution, z_offset, encoder_resolution, encoder_framerate);
    }

    // Forward gocator gocator_properties
    winFifo.enqueue(m);
    auto [y_resolution, x_resolution, z_resolution, z_offset, encoder_resolution, encoder_framerate] = get<GocatorProperties>(get<1>(m));
    store_profile_parameters({y_resolution, x_resolution, z_resolution, 33.0, encoder_resolution, clip_height});

    Adapt<BlockingReaderWriterQueue<msg>> db_fifo{BlockingReaderWriterQueue<msg>()};
    Adapt<BlockingReaderWriterQueue<msg>> dynamic_processing_fifo{BlockingReaderWriterQueue<msg>()};


    std::jthread save_send(save_send_thread, std::ref(db_fifo), std::ref(next));

    std::jthread dynamic_processing;
    std::jthread origin_dectection;
    if (global_config.contains("origin_detector") && global_config["origin_detector"] == "bypass")
    {
      origin_dectection = std::jthread(bypass_fiducial_detection_thread, std::ref(winFifo), std::ref(db_fifo));
    }
    else
    {
      origin_dectection = std::jthread(window_processing_thread, std::ref(winFifo), std::ref(dynamic_processing_fifo));
      origin_dectection = std::jthread(splice_detection_thread, std::ref(winFifo),std::ref(dynamic_processing_fifo));
      dynamic_processing = std::jthread(dynamic_processing_thread, std::ref(dynamic_processing_fifo), std::ref(db_fifo));
    }

    auto iirfilter_left = mk_iirfilterSoS();
    auto iirfilter_right = mk_iirfilterSoS();
    auto iirfilter_left_edge = mk_iirfilterSoS();

    auto delay = mk_delay(global_config["iirfilter"]["delay"]);

    int64_t cnt = 0;

    auto start = std::chrono::high_resolution_clock::now();

    auto dc_filter = mk_dc_filter();
    auto pulley_speed = mk_pulley_stats();

    long drop_profiles = global_config["iirfilter"]["skip"]; // Allow for iir fillter too stablize
    Process_Status status = Process_Status::Finished;

    // auto fn = encoder_distance_id(csp);
    auto stride = global_conveyor_parameters.MaxSpeed / encoder_framerate;
    auto fn = encoder_distance_estimation(winFifo, stride);

    auto pulley_estimator = mk_pulleyfitter(z_resolution, -15.0);
    auto belt_estimator = mk_pulleyfitter(z_resolution, 0.0);

    auto filter_window_len = global_config["sobel_filter"].get<size_t>();

    auto time0 = std::chrono::high_resolution_clock::now();

    auto pulley_rev = mk_pulley_revolution(encoder_framerate);

    do
    {

      gocatorFifo.wait_dequeue(m);
      auto m_id = get<0>(m);

      if (m_id != cads::msgid::scan)
      {
        break;
      }

      ++cnt;

      if (cnt % 20000 == 0)
      {
        std::chrono::time_point  now = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double, std::micro> dt = now - time0;
        time0 = now;
        spdlog::get("cads")->debug("Gocator sending rate : {}", 20000 * (1000000 / dt.count()));
      }

      auto p = get<profile>(get<1>(m));

      if (p.z.size() * x_resolution < x_width * 0.75)
      {
        spdlog::get("cads")->error("Gocator sending profiles with widths less than 0.75 of expected width");
        status = Process_Status::Error;
        break;

      }

      if (p.z.size() < (size_t)width_n)
      {
        spdlog::get("cads")->error("Gocator sending profiles with sample number {} less than {}", p.z.size(), width_n);
        status = Process_Status::Error;
        break;
      }

      double nan_cnt = std::count_if(p.z.begin(), p.z.end(), [](z_element z)
                                     { return std::isnan(z); });

      measurements.send("nancount", 0, nan_cnt);

      if ((nan_cnt / p.z.size()) > nan_percentage)
      {
        spdlog::get("cads")->error("Percentage of nan({}) in profile > {}%", nan_cnt, nan_percentage * 100);
        status = Process_Status::Error;
        break;
      }

      auto iy = p.y;
      auto ix = p.x_off;
      auto iz = p.z;

      auto [pulley_level, pulley_right, ll, lr, cerror] = pulley_levels_clustered(iz, pulley_estimator);

      if (cerror != ClusterError::None)
      {
        // spdlog::get("cads")->debug("Clustering error : {}", ClusterErrorToString(cerror));
        // store_errored_profile(p.z,ClusterErrorToString(cerror));
      }

      iz.insert(iz.begin(), filter_window_len, (float)pulley_level);
      iz.insert(iz.end(), filter_window_len, (float)pulley_right);
      ll += filter_window_len;
      lr += filter_window_len;

      std::fill(iz.begin(), iz.begin() + ll, pulley_level);
      std::fill(iz.begin() + lr, iz.end(), pulley_right);

      nan_interpolation_mean(iz);

      auto pulley_level_filtered = (z_element)iirfilter_left(pulley_level);
      auto pulley_right_filtered = (z_element)iirfilter_right(pulley_right);
      auto left_edge_filtered = (z_element)iirfilter_left_edge(ll);

      measurements.send("pulleylevel", 0, pulley_level_filtered);
      measurements.send("beltedgeposition", 0, ix + left_edge_filtered * x_resolution);

      auto pulley_level_unbias = dc_filter(pulley_level_filtered);

      auto [delayed, dd] = delay({{p.time,iy,ix,iz}, (int)left_edge_filtered, (int)ll, (int)lr, p.z});

      if (!delayed)
        continue;

      if (drop_profiles > 0)
      {
        --drop_profiles;
        continue;
      }

      auto [delayed_profile, left_edge_index_avg, left_edge_index, right_edge_index, raw_z] = dd;
      auto y = delayed_profile.y;
      auto x = delayed_profile.x_off;
      auto z = delayed_profile.z;

      auto gradient = (pulley_right_filtered - pulley_level_filtered) / (double)z.size();
      regression_compensate(z, 0, z.size(), gradient);

      PulleyRevolution ps;
      if (revolution_sensor_config.source == RevolutionSensor::Source::raw)
      {
        ps = pulley_rev(pulley_level);
      }
      else
      {
        ps = pulley_rev(pulley_level_unbias);
      }

      auto [speed, frequency, amplitude] = pulley_speed(ps, pulley_level_unbias, delayed_profile.time);

      if (speed == 0)
      {
        spdlog::get("cads")->error("Belt proabaly stopped.");
        status = Process_Status::Stopped;
        break;

      }

      measurements.send("pulleyspeed", 0, speed);
      measurements.send("pulleyoscillation", 0, amplitude);

      if (cnt % (1000 * 60) == 0)
      {
        auto belt_level = belt_estimator(z_type(z.begin() + left_edge_index, z.begin() + right_edge_index));
        spdlog::get("cads")->info("Pulley Level: {} | Belt Level: {} | Height: {}", pulley_level_filtered, belt_level, belt_level - pulley_level_filtered);
      }

      pulley_level_compensate(z, -pulley_level_filtered, clip_height);
      interpolation_nearest(z.begin() + left_edge_index_avg, z.begin() + right_edge_index, [](float a)
                            { return a == 0; });

      // if(left_edge_index_avg < left_edge_index) {
      //   std::fill(z.begin()+left_edge_index_avg,z.begin() + left_edge_index, interquartile_mean(zrange(z.begin()+left_edge_index,z.begin()+left_edge_index+20)));
      // }

      auto left_edge_index_aligned = left_edge_index_avg;
      auto right_edge_index_aligned = right_edge_index;

      if (z.size() < size_t(left_edge_index_aligned + width_n))
      {
        // spdlog::get("cads")->debug("Belt width({})[la - {}, l- {}, r - {}] less than required. Filled with zeros",z.size(),left_edge_index_aligned,left_edge_index,right_edge_index);
        // store_errored_profile(raw_z);
        const auto cnt = left_edge_index_aligned + width_n - z.size();
        z.insert(z.end(), cnt, interquartile_mean(zrange(z.begin() + right_edge_index_aligned - 20, z.begin() + right_edge_index_aligned - 1)));
      }

      auto f = z | views::take(left_edge_index_aligned + width_n) | views::drop(left_edge_index_aligned);

      if (use_encoder)
      {
        winFifo.enqueue({msgid::scan, cads::profile{delayed_profile.time,y, x + left_edge_index_aligned * x_resolution, {f.begin(), f.end()}}});
      }
      else
      {
        winFifo.enqueue({msgid::scan, cads::profile{delayed_profile.time,y, x + left_edge_index_aligned * x_resolution, {f.begin(), f.end()}}});
        fn.resume({msgid::pulley_revolution_scan,PulleyRevolutionScan{std::get<0>(ps),std::get<1>(ps), cads::profile{delayed_profile.time,y, pulley_level_filtered, {f.begin(), f.end()}}}});
      }

    } while (std::get<0>(m) != msgid::finished);

    winFifo.enqueue({msgid::finished, 0});

    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();

    auto rate = duration != 0 ? (double)cnt / duration : 0;
    spdlog::get("cads")->info("CADS - CNT: {}, DUR: {}, RATE(ms):{} ", cnt, duration, rate);

    gocator->Stop();

    origin_dectection.join();
    spdlog::get("cads")->info("Origin Detection Stopped");

    save_send.join();
    spdlog::get("cads")->info("Upload Thread Stopped");

    return status;
  }
#endif


  



}

namespace cads
{
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

  void process_profile(Io& gocatorFifo, Io& next)
  {

    const auto x_width = global_belt_parameters.Width;
    const auto nan_percentage = global_config["nan_%"].get<double>();
    const auto width_n = (int)global_belt_parameters.WidthN; // gcc-11 on ubuntu cannot compile doubles in ranges expression
    const auto clip_height = global_config["clip_height"].get<z_element>();
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

    auto iirfilter_left = mk_iirfilterSoS();
    auto iirfilter_right = mk_iirfilterSoS();
    auto iirfilter_left_edge = mk_iirfilterSoS();

    auto delay = mk_delay(global_config["iirfilter"]["delay"]);

    int64_t cnt = 0;

    auto start = std::chrono::high_resolution_clock::now();

    auto dc_filter = mk_dc_filter();
    auto pulley_speed = mk_pulley_stats();

    long drop_profiles = global_config["iirfilter"]["skip"]; // Allow for iir fillter too stablize

    auto pulley_estimator = mk_pulleyfitter(z_resolution, -15.0);
    auto belt_estimator = mk_pulleyfitter(z_resolution, 0.0);

    auto filter_window_len = global_config["sobel_filter"].get<size_t>();

    auto time0 = std::chrono::high_resolution_clock::now();

    auto pulley_rev = mk_pulley_revolution();

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

      measurements.send("nancount", 0, nan_cnt);

      if ((nan_cnt / p.z.size()) > nan_percentage)
      {
        spdlog::get("cads")->error("Percentage of nan({}) in profile > {}%", nan_cnt, nan_percentage * 100);
        next.enqueue({msgid::finished,0});
        break;
      }

      auto iy = p.y;
      auto ix = p.x_off;
      auto iz = p.z;

      auto [pulley_level, pulley_right, ll, lr, cerror] = pulley_levels_clustered(iz, pulley_estimator);

      if (cerror != ClusterError::None)
      {
        // spdlog::get("cads")->debug("Clustering error : {}", ClusterErrorToString(cerror));
        // store_errored_profile(p.z,ClusterErrorToString(cerror));
      }

      iz.insert(iz.begin(), filter_window_len, (float)pulley_level);
      iz.insert(iz.end(), filter_window_len, (float)pulley_right);
      ll += filter_window_len;
      lr += filter_window_len;

      std::fill(iz.begin(), iz.begin() + ll, pulley_level);
      std::fill(iz.begin() + lr, iz.end(), pulley_right);

      nan_interpolation_mean(iz);

      auto pulley_level_filtered = (z_element)iirfilter_left(pulley_level);
      auto pulley_right_filtered = (z_element)iirfilter_right(pulley_right);
      auto left_edge_filtered = (z_element)iirfilter_left_edge(ll);

      measurements.send("pulleylevel", 0, pulley_level_filtered);
      measurements.send("beltedgeposition", 0, ix + left_edge_filtered * x_resolution);

      auto pulley_level_unbias = dc_filter(pulley_level_filtered);

      auto [delayed, dd] = delay({{p.time,iy,ix,iz}, (int)left_edge_filtered, (int)ll, (int)lr, p.z});

      if (!delayed)
        continue;

      if (drop_profiles > 0)
      {
        --drop_profiles;
        continue;
      }

      auto [delayed_profile, left_edge_index_avg, left_edge_index, right_edge_index, raw_z] = dd;
      auto y = delayed_profile.y;
      auto x = delayed_profile.x_off;
      auto z = delayed_profile.z;

      auto gradient = (pulley_right_filtered - pulley_level_filtered) / (double)z.size();
      regression_compensate(z, 0, z.size(), gradient);

      PulleyRevolution ps;
      if (revolution_sensor_config.source == RevolutionSensor::Source::raw)
      {
        ps = pulley_rev(pulley_level);
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

      measurements.send("pulleyspeed", 0, speed);
      measurements.send("pulleyoscillation", 0, amplitude);

      if (cnt % (1000 * 60) == 0)
      {
        auto belt_level = belt_estimator(z_type(z.begin() + left_edge_index, z.begin() + right_edge_index));
        spdlog::get("cads")->info("Pulley Level: {} | Belt Level: {} | Height: {}", pulley_level_filtered, belt_level, belt_level - pulley_level_filtered);
      }

      pulley_level_compensate(z, -pulley_level_filtered, clip_height);
      interpolation_nearest(z.begin() + left_edge_index_avg, z.begin() + right_edge_index, [](float a)
                            { return a == 0; });

      // if(left_edge_index_avg < left_edge_index) {
      //   std::fill(z.begin()+left_edge_index_avg,z.begin() + left_edge_index, interquartile_mean(zrange(z.begin()+left_edge_index,z.begin()+left_edge_index+20)));
      // }

      auto left_edge_index_aligned = left_edge_index_avg;
      auto right_edge_index_aligned = right_edge_index;

      if (z.size() < size_t(left_edge_index_aligned + width_n))
      {
        // spdlog::get("cads")->debug("Belt width({})[la - {}, l- {}, r - {}] less than required. Filled with zeros",z.size(),left_edge_index_aligned,left_edge_index,right_edge_index);
        // store_errored_profile(raw_z);
        const auto cnt = left_edge_index_aligned + width_n - z.size();
        z.insert(z.end(), cnt, interquartile_mean(zrange(z.begin() + right_edge_index_aligned - 20, z.begin() + right_edge_index_aligned - 1)));
      }

      auto f = z | views::take(left_edge_index_aligned + width_n) | views::drop(left_edge_index_aligned);

      next.enqueue({msgid::scan, cads::profile{delayed_profile.time,y, x + left_edge_index_aligned * x_resolution, {f.begin(), f.end()}}});

    } while (std::get<0>(m) != msgid::finished);


    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();

    auto rate = duration != 0 ? (double)cnt / duration : 0;
    spdlog::get("cads")->info("CADS - CNT: {}, DUR: {}, RATE(ms):{} ", cnt, duration, rate);

  }


  unique_ptr<GocatorReaderBase> mk_gocator(Io &gocatorFifo)
  {
    auto data_src = global_config["data_source"].get<std::string>();

    if (data_src == "gocator"s)
    {
      spdlog::get("cads")->debug("Using gocator as data source");
      auto gocator = make_unique<GocatorReader>(gocatorFifo);
      return gocator;
    }
    else
    {
      spdlog::get("cads")->debug("Using sqlite as data source");
      auto sqlite = make_unique<SqliteGocatorReader>(gocatorFifo);
      return sqlite;
    }
  }

  void cads_local_main(std::string f) 
  {
    
    auto [L,err] = run_lua_code(f);

    if(err) {
      return;
    }

    auto lua_type = lua_getglobal(L.get(),"mainco");
    
    if(lua_type != LUA_TTHREAD) {
      spdlog::get("cads")->error("TODO");
      return;
    }
    
    auto mainco = lua_tothread(L.get(), -1); 

    while(true)
    {
      lua_pushboolean(mainco,false);
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
        spdlog::get("cads")->error("TODO");
        break;
      }

    }

    auto mainco_status = lua_status(mainco);
    
    if( mainco_status == LUA_OK || mainco_status == LUA_YIELD ) {
      auto nargs = 0;
      lua_pushboolean(mainco,true);
      lua_resume(mainco,L.get(),1,&nargs);
    }
  }

  void cads_remote_main() {
    
    while(true)
    {
      try {
        stop_gocator();
        auto remote_control = remote_control_coro();
        auto [co_err,rmsg] = remote_control.resume(false);
            
        bool restart = false;
        // Wait for start message
        while(true) {
      
          if(co_err) {
            spdlog::get("cads")->error("TODO");
            stop_gocator();
            restart = true;
            break;
          }

          if(rmsg.index() == 0) break;

          std::tie(co_err,rmsg) = remote_control.resume(false);
        }

        if(restart) continue;

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

        while(true)
        {
          lua_pushboolean(mainco,false);
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
            spdlog::get("cads")->error("TODO");
            break;
          }

          auto [co_err,rmsg] = remote_control.resume(false);

          if(co_err) {
            spdlog::get("cads")->error("TODO");
            break;
          }

          if(rmsg.index() != 2) break;

        }
        auto mainco_status = lua_status(mainco);
        
        if( mainco_status == LUA_OK || mainco_status == LUA_YIELD ) {
          auto nargs = 0;
          lua_pushboolean(mainco,true);
          lua_resume(mainco,L.get(),1,&nargs);
        }

      }catch(std::exception ex) {
        spdlog::get("cads")->error("TODO");
      }
    }

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

    Adapt<BlockingReaderWriterQueue<msg>> gocatorFifo{BlockingReaderWriterQueue<msg>(4096 * 1024)};

    auto gocator = mk_gocator(gocatorFifo);
    gocator->Start();

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
  }

  
  void stop_gocator()
  {
    Adapt<BlockingReaderWriterQueue<msg>> gocatorFifo{BlockingReaderWriterQueue<msg>(4096 * 1024)};
    auto gocator = mk_gocator(gocatorFifo);
    gocator->Stop();
  }

  void dump_gocator_log()
  {
    Adapt<BlockingReaderWriterQueue<msg>> gocatorFifo{BlockingReaderWriterQueue<msg>(4096 * 1024)};
    GocatorReader gocator(gocatorFifo);
    gocator.Log();
  }

}
