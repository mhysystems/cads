#include <opencv2/core.hpp>
#include <opencv2/core/types.hpp>
#include <opencv2/imgproc.hpp>

#include <cads.h>
#include <regression.h>

#include <db.h>
#include <coms.h>
#include <constants.h>
#include <fiducial.h>
#include <readerwriterqueue.h>
#include <gocator_reader.h>
#include <sqlite_gocator_reader.h>

#include <exception>
#include <limits>
#include <string>
#include <vector>
#include <tuple>
#include <algorithm>
#include <thread>
#include <memory>
#include <ranges>
#include <chrono>
#include <sstream>
#include <fstream>
#include <iostream>

#include <spdlog/spdlog.h>

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
#include <intermessage.h>
#include <belt.h>
#include <interpolation.h>
#include <upload.h>

using namespace std;
using namespace moodycamel;
using namespace std::chrono;
using CadsMat = cv::UMat; // cv::cuda::GpuMat


namespace
{
  using namespace cads;

  void upload_conveyorbelt_parameters() {
    
    auto [conveyor_id,err] = fetch_conveyor_id();
    
    if(!err && conveyor_id == 0){
      auto [new_id,err] = remote_addconveyor(global_conveyor_parameters);
      if(!err) {
        store_conveyor_id(new_id);
        conveyor_id = new_id;
      }
    }

    auto [belt_id,errb] = fetch_belt_id();
    
    if(!errb && belt_id == 0 && conveyor_id != 0){
      auto belt = global_belt_parameters;
      belt.Conveyor = conveyor_id;
      auto [new_id,err] = remote_addbelt(belt);
      
      if(!err) {
        store_belt_id(new_id);
        belt_id = new_id;
      }
    }
  }

  std::tuple<unique_ptr<GocatorReaderBase>,bool> mk_gocator(BlockingReaderWriterQueue<msg> &gocatorFifo, bool force_gocator = false, bool use_encoder = false)
  {
    auto data_src = global_config["data_source"].get<std::string>();

    if (data_src == "gocator"s || force_gocator)
    {
      bool connect_via_ip = global_config.contains("gocator_ip");
      spdlog::get("cads")->debug("Using gocator as data source");
      auto gocator = connect_via_ip ? make_unique<GocatorReader>(gocatorFifo, use_encoder, global_config.at("gocator_ip"s).get<std::string>()) : make_unique<GocatorReader>(gocatorFifo,use_encoder);
      return {std::move(gocator),true};
    }
    else
    {
      spdlog::get("cads")->debug("Using sqlite as data source");
      auto sqlite = make_unique<SqliteGocatorReader>(gocatorFifo);
      return {std::move(sqlite),false};
    }
  }

  enum class Process_Status {Error, Finished, Stopped};

  Process_Status process_impl(BlockingReaderWriterQueue<msg> &next)
  {

    std::jthread uploading_conveyorbelt_parameters(upload_conveyorbelt_parameters);

    BlockingReaderWriterQueue<msg> gocatorFifo(4096 * 1024);
    BlockingReaderWriterQueue<msg> winFifo(4096 * 1024);

    const auto x_width = global_belt_parameters.Width;
    const auto nan_percentage = global_config["nan_%"].get<double>();
    const auto width_n = global_config["width_n"].get<int>();
    const auto clip_height = global_config["clip_height"].get<z_element>();
    const auto use_encoder = global_config["use_encoder"].get<bool>();

    auto [gocator,is_gocator] = mk_gocator(gocatorFifo,use_encoder);
    gocator->Start();

    cads::msg m;
    gocatorFifo.wait_dequeue(m);
    auto m_id = get<0>(m);

    if(m_id == cads::msgid::finished) {
      return Process_Status::Finished;
    }

    if (m_id != cads::msgid::gocator_properties)
    {
      std::throw_with_nested(std::runtime_error("preprocessing:First message must be gocator_properties"));
    }else {
      auto [y_resolution, x_resolution, z_resolution, z_offset, encoder_resolution, encoder_framerate] = get<GocatorProperties>(get<1>(m));
      spdlog::get("cads")->info("Gocator y_resolution:{}, x_resolution:{}, z_resolution:{}, z_offset:{}, encoder_resolution:{}, encoder_framerate:{}",y_resolution, x_resolution, z_resolution, z_offset, encoder_resolution, encoder_framerate);
    }

    // Forward gocator gocator_properties
    winFifo.enqueue(m);
    auto [y_resolution, x_resolution, z_resolution, z_offset, encoder_resolution, encoder_framerate] = get<GocatorProperties>(get<1>(m));
    store_profile_parameters({y_resolution, x_resolution, z_resolution, 33.0, encoder_resolution, clip_height});

    BlockingReaderWriterQueue<msg> db_fifo;
    BlockingReaderWriterQueue<msg> dynamic_processing_fifo;

    bool terminate_publish = false;
    std::jthread realtime_publish(realtime_publish_thread, std::ref(terminate_publish));
    std::jthread save_send(save_send_thread, std::ref(db_fifo),std::ref(next));
    
    std::jthread dynamic_processing;
    std::jthread origin_dectection;
    if(global_config.contains("origin_detector") && global_config["origin_detector"] == "bypass") {
      origin_dectection = std::jthread(bypass_fiducial_detection_thread,std::ref(winFifo), std::ref(db_fifo));
    }else {
      auto y_res = encoder_framerate != 0.0 ? global_conveyor_parameters.MaxSpeed / encoder_framerate : y_resolution;
      origin_dectection = std::jthread(window_processing_thread, x_resolution, y_res, width_n, std::ref(winFifo), std::ref(dynamic_processing_fifo));
      dynamic_processing = std::jthread(dynamic_processing_thread, std::ref(dynamic_processing_fifo), std::ref(db_fifo), width_n);
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

    auto csp = [&](const profile &p) {
      winFifo.enqueue({msgid::scan, p});
    };

    //auto fn = encoder_distance_id(csp); 
    auto stride = global_conveyor_parameters.MaxSpeed / encoder_framerate;
    auto fn = encoder_distance_estimation(csp,stride); 

    auto pulley_estimator = mk_pulleyfitter(z_resolution,-15.0);
    auto belt_estimator = mk_pulleyfitter(z_resolution,0.0);

    auto filter_window_len = global_config["sobel_filter"].get<size_t>();

    auto time0 = std::chrono::high_resolution_clock::now();
    
    auto pulley_rev =  mk_pulley_revolution();

    do
    {
      
      gocatorFifo.wait_dequeue(m);
      auto m_id = get<0>(m);

      if (m_id != cads::msgid::scan)
      {
        break;
      }
      
      ++cnt;

      if(cnt % 20000 == 0) {
        auto now = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double, std::micro> dt = now - time0;
        time0 = now;
        spdlog::get("cads")->debug("Gocator sending rate : {}", 20000*(1000000 / dt.count()));
      }

      auto p = get<profile>(get<1>(m));
      
      auto iy = p.y;
      auto ix = p.x_off;
      auto iz = p.z;

      //spike_filter(iz);
      auto [pulley_level,pulley_right,ll,lr,clusters,cerror] = pulley_levels_clustered(iz,pulley_estimator);
      
      if(cerror != ClusterError::None) {
        //spdlog::get("cads")->debug("Clustering error : {}", ClusterErrorToString(cerror));
        //store_errored_profile(p.z,ClusterErrorToString(cerror));
      }

      recontruct_z(iz,clusters);
      iz.insert(iz.begin(),filter_window_len,(float)pulley_level);
      iz.insert(iz.end(),filter_window_len,(float)pulley_right);

      ll += filter_window_len;
      lr += filter_window_len;

      if (iz.size()*x_resolution < x_width * 0.75)
      {
        spdlog::get("cads")->error("Gocator sending profiles with widths less than 0.75 of expected width");
        if(is_gocator) {
          status = Process_Status::Error;
          break;
        }else {
          continue;
        }
      }

      if (iz.size() < (size_t)width_n)
      {
        spdlog::get("cads")->error("Gocator sending profiles with sample number {} less than {}", iz.size(), width_n);
        if(is_gocator) {
          status = Process_Status::Error;
          break;
        }else {
          continue;
        }
      }

      double nan_cnt = std::count_if(iz.begin(), iz.end(), [](z_element z)
                                   { return std::isnan(z); });

      if ((nan_cnt / iz.size()) > nan_percentage )
      {
        spdlog::get("cads")->error("Percentage of nan({}) in profile > {}%", nan_cnt,nan_percentage * 100);
        if(is_gocator) {
          status = Process_Status::Error;
          break;
        }else {
          continue;
        }
      }

      nan_interpolation_last(iz);

      auto [pre_left_edge_index, pre_right_edge_index] = find_profile_edges_sobel(iz);


      if(std::abs(pre_left_edge_index - (int)ll) > 2) {
        //spdlog::get("cads")->debug("sobel & dbscan don't match: sobel({},{}) dbscan({},{})", pre_left_edge_index,pre_right_edge_index,ll,lr);
        //store_errored_profile(p.z,"sobel");
      }

      auto pulley_level_filtered = (z_element)iirfilter_left(pulley_level);
      auto pulley_right_filtered = (z_element)iirfilter_right(pulley_right);
      auto left_edge_filtered = (z_element)iirfilter_left_edge(ll);
      
      auto pulley_level_unbias = dc_filter(pulley_level_filtered);

      auto [delayed, dd] = delay({iy, ix, iz, (int)left_edge_filtered,(int)ll, (int)pre_right_edge_index,p.z});

      if (!delayed)
        continue;

      if (drop_profiles > 0)
      {
        --drop_profiles;
        continue;
      }

      auto [y, x, z, left_edge_index_avg, left_edge_index, right_edge_index, raw_z] = dd;

      auto gradient = (pulley_right_filtered - pulley_level_filtered) / (double)z.size();
      regression_compensate(z, 0, z.size(), gradient);
      
      PulleyRevolution ps;
      if(revolution_sensor_config.source == RevolutionSensor::Source::raw) {
        ps = pulley_rev(pulley_level);
      }else {
        ps = pulley_rev(pulley_level_unbias);
      }

      auto [speed,frequency,amplitude] = pulley_speed(ps,pulley_level_unbias);

      if (speed == 0)
      {
        spdlog::get("cads")->error("Belt proabaly stopped.");
        
        if(is_gocator) {
          status = Process_Status::Stopped;
          break;
        }
      }

      if (cnt % (1000 * 20) == 0)
      {
        publish_PulleyOscillation(amplitude);
        publish_SurfaceSpeed(speed);
        

        spdlog::get("cads")->debug("Pulley Oscillation(mm): {}",amplitude);
        spdlog::get("cads")->debug("Pulley Frequency(Hz): {}", frequency);
        spdlog::get("cads")->debug("Surface Speed(m/s): {}", speed);
      }

      if (cnt % (1000 * 60) == 0)
      {
        auto belt_level = belt_estimator(z_type(z.begin()+left_edge_index,z.begin()+right_edge_index));
        spdlog::get("cads")->info("Pulley Level: {} | Belt Level: {} | Height: {}", pulley_level_filtered,belt_level, belt_level - pulley_level_filtered);
      }

      if (cnt % (1000 * 5 * 60) == 0)
      {
        //auto s = fmt::format("raw_{:.1f}",pulley_level_filtered);
        //store_errored_profile(raw_z,s);  
      }

      pulley_level_compensate(z, -pulley_level_filtered, clip_height);

      if(left_edge_index_avg < left_edge_index) {
        std::fill(z.begin()+left_edge_index_avg,z.begin() + left_edge_index,z[left_edge_index]);
      }
      
      auto [left_edge_index_aligned,right_edge_index_aligned] = find_profile_edges_zero(z);
      
      if(z.size() < size_t(left_edge_index_aligned + width_n)) {
        //spdlog::get("cads")->debug("Belt width({})[la - {}, l- {}, r - {}] less than required. Filled with zeros",z.size(),left_edge_index_aligned,left_edge_index,right_edge_index);
        //store_errored_profile(raw_z);
        const auto cnt = left_edge_index_aligned + width_n - z.size();
        z.insert(z.end(),cnt,z[right_edge_index_aligned-1]);
      }

      auto f = z | views::take(left_edge_index_aligned + width_n) | views::drop(left_edge_index_aligned);

      if(use_encoder) 
      {
        winFifo.enqueue({msgid::scan, cads::profile{y, x + left_edge_index_aligned * x_resolution, {f.begin(), f.end()}}});
      }
      else 
      {
        fn.resume({ps,cads::profile{y, pulley_level_filtered, {f.begin(), f.end()}}});
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

    terminate_publish = true;
    realtime_publish.join();
    spdlog::get("cads")->info("Realtime publishing Thread Stopped");

    return status;
  }

}


namespace cads
{

  void upload_profile_only(std::string params, std::string db_name)
  {
    auto db = db_name.empty() ? global_config["profile_db_name"].get<std::string>() : db_name;
    
    if(params == "") {
      spdlog::get("cads")->error("--upload argument need to be supplied in the form of [revid,first_idx,last_idx]");
    }else {
      
      auto args = nlohmann::json::parse(params);
      auto [rev_id,first_idx,last_idx] = args.get<std::tuple<int,int,int>>();
      auto fetch_belt = fetch_belt_coro(0,last_idx,first_idx,256,db);
      auto [belt_id, belt_id_err] = fetch_belt_id();

      if(belt_id_err && belt_id == 0) {
        spdlog::get("cads")->info("Belt is not registered with server");
        return;
      }

      meta m;

      auto [params, err] = fetch_profile_parameters(db);
      auto [Ymin,Ymax,YmaxN,WidthN,err2] = fetch_belt_dimensions(rev_id,last_idx,db);
      auto now = chrono::floor<chrono::seconds>(date::utc_clock::now()); // Default sends to much decimal precision for asp.net core
      auto ts = date::format("%FT%TZ", now);
      
      m.chrono = ts;
      m.conveyor = global_conveyor_parameters.Name;
      m.site = global_conveyor_parameters.Site;
      m.WidthN = WidthN;
      m.x_res = params.x_res;
      m.y_res = params.y_res;
      m.Ymax = Ymax;
      m.YmaxN = YmaxN;
      m.z_off = params.z_off;
      m.z_res = params.z_res;
      m.Belt = belt_id;

      auto post_profile = post_profiles_coro(m);

      while (true)
      {
        auto [co_terminate, cv] = fetch_belt.resume(0);
        auto [idx, p] = cv;

        if (co_terminate)
        {
          break;
        }

        if(p.z.size() < size_t(WidthN)) {
          const auto cnt = WidthN - p.z.size();
          p.z.insert(p.z.end(),cnt,0);
        }

        auto f = p.z | views::take(int(WidthN));
        post_profile.resume(cads::profile{p.y,p.x_off,{f.begin(), f.end()}});


      }

    }
  }

  void store_profile_only()
  {
    auto terminate_subscribe = false;
    moodycamel::BlockingConcurrentQueue<int> nats_queue;
    BlockingReaderWriterQueue<msg> gocatorFifo;
    std::jthread remote_control(remote_control_thread,std::ref(nats_queue),std::ref(terminate_subscribe));

    auto [gocator, is_gocator] = mk_gocator(gocatorFifo);

    gocator->Start();

    cads::msg m;

    gocatorFifo.wait_dequeue(m);

    if (get<0>(m) != cads::msgid::gocator_properties)
    {
      std::throw_with_nested(std::runtime_error("First message must be resolutions"));
    }else {
      auto now = chrono::floor<chrono::milliseconds>(date::utc_clock::now()); 
      auto ts = date::format("%FT%TZ", now);
      spdlog::get("cads")->info("Let's go! - {}",ts);
    }

    auto [y_resolution, x_resolution, z_resolution, z_offset, encoder_resolution, gocator_framerate] = get<GocatorProperties>(get<1>(m));
    store_profile_parameters({y_resolution, x_resolution, z_resolution, z_offset, encoder_resolution, global_config["clip_height"].get<double>()});

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
        if(nats_queue.try_dequeue(ignore)) {
          reset_y = p.y;
          p.y = 0;
          auto now = chrono::floor<chrono::milliseconds>(date::utc_clock::now()); 
          auto ts = date::format("%FT%TZ", now);
          spdlog::get("cads")->info("Y reset! - {}",ts);
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
  }

  

  void process()
  {
    BlockingReaderWriterQueue<msg> scan_upload_fifo(4096 * 1024);
    std::jthread save_send(upload_scan_thread, std::ref(scan_upload_fifo));

    for (int sleep_wait = 1;;)
    {
      
      auto start = std::chrono::high_resolution_clock::now();

      auto status = process_impl(scan_upload_fifo);

      auto now = std::chrono::high_resolution_clock::now();
      auto period = std::chrono::duration_cast<std::chrono::seconds>(now - start).count();

      if (period > sleep_wait * 60)
      {
        sleep_wait = 1;
      }

      if (status == Process_Status::Error)
      {
        spdlog::get("cads")->info("Sleeping for {} minutes",sleep_wait);
        std::this_thread::sleep_for(std::chrono::seconds(sleep_wait * 60));

        if (sleep_wait < 32)
        {
          sleep_wait *= 2;
        }
      }
      else if(status == Process_Status::Stopped) {
        spdlog::get("cads")->info("Sleeping for {} seconds",30);
        std::this_thread::sleep_for(std::chrono::seconds(30));
      }
      else
      {
        break;
      }
    }
    
    scan_upload_fifo.enqueue({msgid::finished,0});
  }

  void generate_signal()
  {

    BlockingReaderWriterQueue<msg> gocatorFifo(4096 * 1024);

    auto [gocator,is_gocator] = mk_gocator(gocatorFifo);
    gocator->Start();

    auto iirfilter = mk_iirfilterSoS();
    auto delay = mk_delay(global_config["iirfilter"]["delay"]);

    cads::msg m;

    gocatorFifo.wait_dequeue(m);
    auto m_id = get<0>(m);

    if(m_id == cads::msgid::finished) {
      return;
    }

    if (m_id != cads::msgid::gocator_properties)
    {
      std::throw_with_nested(std::runtime_error("preprocessing:First message must be gocator_properties"));
    }

    auto [y_resolution, x_resolution, z_resolution, z_offset, encoder_resolution, encoder_framerate] = get<GocatorProperties>(get<1>(m));

    auto differentiation = mk_dc_filter(); 

    std::ofstream filt("filt.txt");

    
    
    auto pulley_estimator = mk_pulleyfitter(z_resolution,-212.0);

    do
    {

      gocatorFifo.wait_dequeue(m);
      auto m_id = get<0>(m);

      if (m_id == cads::msgid::scan)
      {

        auto p = get<profile>(get<1>(m));
        auto iy = p.y;
        auto ix = p.x_off;
        auto iz = p.z;

        auto [pulley_left,pulley_right,ll,lr,clusters,cerror] = pulley_levels_clustered(p.z,pulley_estimator);

        auto bottom_avg = pulley_left;
        auto bottom_filtered = iirfilter(bottom_avg);

        filt << bottom_avg << "," << bottom_filtered << '\n';
        filt.flush();

        auto [delayed, dd] = delay({iy, ix, iz, 0,0, 0,p.z});
        if (!delayed)
          continue;

        auto [y, x, z, ignore_d, ignore_a, ignore_b,ignore_c] = dd;
      }

    } while (std::get<0>(m) != msgid::finished);

    gocator->Stop();
    spdlog::get("cads")->info("Gocator Stopped");
  }

  void stop_gocator()
  {

    BlockingReaderWriterQueue<msg> f;
    GocatorReader gocator(f);
    gocator.Stop();
  }

  void dump_gocator_log() {
    BlockingReaderWriterQueue<msg> gocatorFifo(4096 * 1024);
    GocatorReader gocator(gocatorFifo);
    gocator.Log();
  }

}
