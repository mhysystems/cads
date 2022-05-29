#include <opencv2/core.hpp>
#include <opencv2/core/types.hpp>
#include <opencv2/imgproc.hpp>

#include <spdlog/spdlog.h>
#include <spdlog/sinks/rotating_file_sink.h>
#include <spdlog/sinks/stdout_color_sinks.h>

#include <csv.hpp>
#include <cads.h>
#include <regression.h>
#include <nlohmann/json.hpp>
#include <db.h>
#include <upload.h>
#include <constants.h>
#include <fiducial.h>
#include <nan_removal.h>
#include <window.hpp>
#include <readerwriterqueue.h>
#include <gocator_reader.h>
#include <sqlite_gocator_reader.h>

#include <z_data_generated.h>
#include <p_config_generated.h>

#include <exception>
#include <limits>
#include <string>
#include <vector>
#include <tuple>
#include <algorithm>
#include <deque>
#include <thread>
#include <memory>
#include <ranges>
#include <chrono>
#include <sstream>

#include <filters.h>
#include <edge_detection.h>
#include <date/date.h>
#include <date/tz.h>
#include <fmt/core.h>
#include <fmt/chrono.h>

#include <global_config.h>

#include <coro.hpp>

using namespace std;
using namespace moodycamel;
using json = nlohmann::json;
using namespace std::chrono;
using CadsMat = cv::UMat; // cv::cuda::GpuMat

spdlog::logger cadslog("cads", {std::make_shared<spdlog::sinks::rotating_file_sink_st>("cads.log", 1024 * 1024 * 5, 1), std::make_shared<spdlog::sinks::stdout_color_sink_st>()});

namespace cads
{

  void save_send_thread(BlockingReaderWriterQueue<profile> &profile_fifo, std::string ts)
  {
    profile p;
    profile_fifo.wait_dequeue(p);

    if (p.y == std::numeric_limits<y_type>::max())
      return;

    BlockingReaderWriterQueue<y_type> upload_fifo;

    std::jthread upload(http_post_thread_bulk, std::ref(upload_fifo), ts);

    auto store_profile = store_profile_coro(p);

    while (true)
    {

      auto [invalid, y] = store_profile(p);
      if (!invalid)
        upload_fifo.enqueue(y);

      if (p.y == std::numeric_limits<y_type>::max())
      {
        break;
      }

      profile_fifo.wait_dequeue(p);
    }
    upload_fifo.enqueue(std::numeric_limits<y_type>::max());
    cadslog.info("Stopping save_send_thread");
  }

  tuple<z_element, z_element> barrel_offset(int cnt, double z_resolution, BlockingReaderWriterQueue<msg> &ps)
  {

    window win;
    const auto z_height_mm = global_config["z_height"].get<double>();
    cads::msg m;

    do
    {
      ps.wait_dequeue(m);
      auto m_id = get<0>(m);

      if (m_id == msgid::scan)
      {
        win.push_back(get<profile>(get<1>(m)));
      }
      else
      {
        break;
      }

    } while (--cnt > 0);

    if (cnt != 0)
    {
      std::throw_with_nested(std::runtime_error("barrel_offset:Less profiles in fifo than required by argument cnt"));
    }

    auto [z_min, z_max] = find_minmax_z(win);

    // Histogram, first is z value, second is count
    auto hist = histogram(win, z_min, z_max);

    const auto peak = get<0>(hist[0]);
    const auto thickness = z_height_mm / z_resolution;

    // Remove z values greater than the peak minus approx belt thickness.
    // Assumes the next peak will be the barrel values
    auto f = hist | views::filter([thickness, peak](tuple<double, z_element> a)
                                  { return peak - get<0>(a) > thickness; });
    vector<tuple<double, z_element>> barrel(f.begin(), f.end());

    return {get<0>(barrel[0]), peak};
  }

  double belt_regression(int cnt, BlockingReaderWriterQueue<msg> &ps)
  {

    const int nan_num = global_config["left_edge_nan"].get<int>();
    const auto z_height_mm = global_config["z_height"].get<double>();
    const int spike_window_size = nan_num / 3;
    double gradient = 0.0, intercept = 0.0;
    const double n = cnt - 1;
    cads::msg m;

    do
    {
      ps.wait_dequeue(m);
      auto m_id = get<0>(m);

      if (m_id == msgid::scan)
      {
        auto p = get<profile>(get<1>(m));
        spike_filter(p.z, spike_window_size);
        auto [left_edge_index, right_edge_index] = find_profile_edges_nans_outer(p.z, nan_num);
        nan_filter(p.z);
        auto [i, g] = linear_regression(p.z, left_edge_index, right_edge_index);
        intercept += i;
        gradient += g;
      }
      else
      {
        break;
      }

    } while (--cnt > 0);

    if (cnt != 0)
    {
      std::throw_with_nested(std::runtime_error("belt_regression:Less profiles in fifo than required by argument cnt"));
    }

    return gradient / n;
  }

  unique_ptr<GocatorReaderBase> mk_gocator(BlockingReaderWriterQueue<msg> &gocatorFifo)
  {
    auto data_src = global_config["data_source"].get<std::string>();

    if (data_src == "gocator"s)
    {
      cadslog.debug("Using gocator as data source");
      return make_unique<GocatorReader>(gocatorFifo);
    }
    else
    {
      cadslog.debug("Using sqlite as data source");
      return make_unique<SqliteGocatorReader>(gocatorFifo);
    }
  }

  void process_daily()
  {
    using namespace date;
    using namespace chrono;

    auto sts = global_config["daily_start_time"].get<std::string>();

    do
    {
      if (sts != "now"s)
      {
        auto now = current_zone()->to_local(system_clock::now());
        auto today = chrono::floor<chrono::days>(now);
        auto daily_time = duration_cast<seconds>(now - today);

        std::stringstream st{sts};

        system_clock::duration run_in;
        st >> parse("%T", run_in);

        auto rest = (24h - (daily_time - run_in)) % 24h;
        auto d = fmt::format("{:%T}", rest);
        cadslog.info("Sleep For {}", d);
        this_thread::sleep_for(rest);
      }
      process_one_revolution();

    } while (sts != "now"s);
  }

  void store_profile_only()
  {

    auto db_name = global_config["db_name"].get<std::string>();
    create_db(db_name);

    auto data_src = global_config["data_source"].get<std::string>();
    BlockingReaderWriterQueue<msg> gocatorFifo;

    auto gocator = mk_gocator(gocatorFifo);

    gocator->Start();

    auto [db, stmt] = open_db(db_name);

    auto y_max_length = global_config["y_max_length"].get<double>();
    auto start = std::chrono::high_resolution_clock::now();
    double Y = 0.0;
    cads::msg m;

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
        store_profile(stmt, p);
        break;
      }
      case cads::msgid::resolutions:
      {
        auto [y_resolution, x_resolution, z_resolution, z_offset, encoder_resolution] = get<resolutions_t>(get<1>(m));
        cadslog.info("Gocator contants - y_res:{}, x_res:{}, z_res:{}, z_off:{}, encoder_res:{}", y_resolution, x_resolution, z_resolution, z_offset, encoder_resolution);
        store_profile_parameters(y_resolution, x_resolution, z_resolution, z_offset, encoder_resolution);
        break;
      }
      }

    } while (get<0>(m) != msgid::finished && Y < y_max_length);

    gocator->Stop();
    close_db(db, stmt);
  }

  void shift_Mat(cv::Mat &m)
  {
    for (int i = m.rows - 1; i > 0; --i)
    {
      memcpy(m.ptr<float>(i), m.ptr<float>(i - 1), m.cols * sizeof(float));
    }
  }

  coro<profile, profile> window_processing_coro(double x_resolution, double y_resolution, double z_resolution)
  {

    auto fiducial = make_fiducial(x_resolution, y_resolution);
    window profile_buffer;

    auto fdepth = global_config["fiducial_depth"].get<double>() / z_resolution;
    double lowest_correlation = std::numeric_limits<double>::max();
    const double belt_crosscorr_threshold = global_config["belt_cross_correlation_threshold"].get<double>();

    // delay
    for (int j = 0; j < fiducial.rows; j++)
    {
      auto profile = co_yield std::move(null_profile);
      profile_buffer.push_back(profile);
    }

    cv::Mat belt = window_to_mat(profile_buffer, x_resolution);
    if (!belt.isContinuous())
    {
      std::throw_with_nested(std::runtime_error("window_processing:OpenCV matrix must be continuous for row shifting using memcpy"));
    }

    while (true)
    {
      y_type y = profile_buffer.front().y;
      const auto cv_threshhold = left_edge_avg_height(belt, fiducial) - fdepth;
      auto correlation = search_for_fiducial(belt, fiducial, cv_threshhold);
      //correlation += 1.0;
      lowest_correlation = std::min(lowest_correlation, correlation);

      if (correlation < belt_crosscorr_threshold)
      {
        cadslog.info("Correlation : {} at y : {}", correlation, y);
        fiducial_as_image(belt);
        // if (!find_first_origin && !loop_forever)
        //   break;

        // find_first_origin = false;
        // frame_offset += profile_buffer.front().y;
 
        // Reset buffer y values to origin
        for (auto off = y; auto &p : profile_buffer)
        {
          p.y -= off;
        }

        lowest_correlation = std::numeric_limits<double>::max();
      }

      auto profile = co_yield std::move(profile_buffer.front());
      shift_Mat(belt);

      auto m_i = belt.ptr<float>(0);

      for (int i = 0; i < std::min(belt.cols, (int)profile.z.size()); ++i)
      {
        m_i[i] = profile.z[i];
      }

      if (profile.z.size() < belt.cols)
      {
        for (int i = profile.z.size(); i < belt.cols; ++i)
        {
          m_i[i] = NAN;
        }
      }

      profile_buffer.pop_front();
      profile_buffer.push_back(profile);
    }
  }

  std::tuple<double, double, double, z_element> preprocessing(BlockingReaderWriterQueue<msg> &gocatorFifo, std::string ts)
  {
    cads::msg m;

    gocatorFifo.wait_dequeue(m);
    auto m_id = get<0>(m);

    if (m_id != cads::msgid::resolutions)
    {
      std::throw_with_nested(std::runtime_error("preprocessing:First message must be resolutions"));
    }

    auto [y_resolution, x_resolution, z_resolution, z_offset, encoder_resolution] = get<resolutions_t>(get<1>(m));
    cadslog.info("Gocator contants - y_res:{}, x_res:{}, z_res:{}, z_off:{}, encoder_res:{}", y_resolution, x_resolution, z_resolution, z_offset, encoder_resolution);
    auto [bottom, top] = barrel_offset(1024, z_resolution, gocatorFifo);
    store_profile_parameters(y_resolution, x_resolution, z_resolution, -bottom * z_resolution, encoder_resolution);
    std::thread([=]()
                { http_post_profile_properties(y_resolution, x_resolution, z_resolution, -bottom * z_resolution, ts); })
        .detach();

    return {x_resolution, y_resolution, z_resolution, bottom};
  }

  void process_one_revolution()
  {

    BlockingReaderWriterQueue<msg> gocatorFifo(4096 * 1024);

    auto gocator = mk_gocator(gocatorFifo);
    gocator->Start();

    BlockingReaderWriterQueue<profile> db_fifo;
    auto ts = fmt::format("{:%F-%H-%M}", std::chrono::system_clock::now());
    std::jthread save_send(save_send_thread, std::ref(db_fifo), ts);

    auto [x_resolution, y_resolution, z_resolution, bottom] = preprocessing(gocatorFifo, ts);

    auto y_max_length = global_config["y_max_length"].get<double>();
    const auto z_height_mm = global_config["z_height"].get<double>();
    const int nan_num = global_config["left_edge_nan"].get<int>();
    const int spike_window_size = nan_num / 4;

    auto origin_dectection = window_processing_coro(x_resolution, y_resolution, z_resolution);

    auto gradient = belt_regression(64, gocatorFifo);

    auto iirfilter = mk_iirfilterSoS();
    auto delay = mk_delay(global_config["iirfilter"]["delay"]);

    uint64_t cnt = 0, frame_count = 0;
    y_type frame_offset = 0;
    bool find_first_origin = true, loop_forever = global_config["loop_forever"].get<bool>();

    double lowest_correlation = std::numeric_limits<double>::max();

    auto start = std::chrono::high_resolution_clock::now();

    cads::msg m;

    do
    {
     
      gocatorFifo.wait_dequeue(m);
      auto m_id = get<0>(m);

      if (m_id != cads::msgid::scan)
      {
        break;
      }

      auto p = get<profile>(get<1>(m));
      auto iy = p.y;
      auto ix = p.x_off;
      auto iz = p.z;
      ++cnt;

      spike_filter(iz, spike_window_size);
      auto [bottom_avg, top_avg] = barrel_offset(iz, z_resolution, z_height_mm);
      auto bottom_filtered = iirfilter(bottom_avg);

      auto [delayed, dd] = delay({iy, ix, iz});
      if (!delayed)
        continue;

      auto [y, x, z] = dd;

      auto [left_edge_index, right_edge_index] = find_profile_edges_nans_outer(z, nan_num);

      nan_filter(z);
      regression_compensate(z, left_edge_index, right_edge_index, gradient);

      barrel_height_compensate(z, bottom - bottom_filtered);

      auto f = z | views::take(right_edge_index) | views::drop(left_edge_index);

      auto [ig, profile] = origin_dectection({y - frame_offset, x + left_edge_index * x_resolution, {f.begin(), f.end()}});

      
#if 0      
      if (!find_first_origin)
      {
        ++frame_count;
        db_fifo.enqueue(profile);
      }


        if (profile.y > y_max_length)
        {
          cadslog.info("Origin not found before Max samples. Lowest Correlation : {}", lowest_correlation);
          frame_offset = y - profile_buffer.size() + 1;
          frame_count = 0;
          // Reset buffer y values to origin
          for (int i = 0; auto &p : profile_buffer)
          {
            p.y = i++;
          }

          find_first_origin = true;
          lowest_correlation = std::numeric_limits<double>::max();

          if (!loop_forever)
            break;
        }
#endif

    } while (std::get<0>(m) != msgid::finished);

    db_fifo.enqueue({std::numeric_limits<y_type>::max(), NAN, {}});

    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
    cadslog.info("CADS - CNT: {}, DUR: {}, RATE(ms):{} ", cnt, duration, (double)cnt / duration);

    gocator->Stop();
    cadslog.info("Gocator Stopped");

    save_send.join();
    cadslog.info("Upload Thread Stopped");
  }

  void process_experiment()
  {
  }

  void stop_gocator()
  {

    BlockingReaderWriterQueue<msg> f;
    GocatorReader gocator(f);
    gocator.Stop();
  }

#if 0
 void process_flatbuffers5()
	{
				
		BlockingReaderWriterQueue<char> gocatorFifo;
		GocatorReader gocator(gocatorFifo);
		gocator.Start();

		// Must be first access to in_file; These values get written once
		auto [y_resolution,x_resolution,z_resolution,z_offset] = get_gocator_constants(std::ref(gocatorFifo));

		json resolution;
	
		resolution["y"] = y_resolution;
		resolution["x"] = x_resolution;
		resolution["z"] = z_resolution;
		resolution["z_off"] = z_offset;

    const double z_height = global_config["z_height"].get<double>();
    const auto y_max_length = global_config["y_max_length"].get<uint64_t>();
		
		spdlog::info("Gocator constants {}", resolution.dump());
			
		auto recorder_data = get_flatworld(std::ref(gocatorFifo));
    auto cnt = 129; //y_max_length;
    deque<profile> profile_buffer;
    const int nan_num = global_config["left_edge_nan"].get<int>();

    const uint64_t y_max_samples = (uint64_t)(global_config["y_max_length"].get<double>()/y_resolution);
    
		auto [db, stmt] = open_db();
		auto fetch_stmt = fetch_profile_statement(db);

		std::condition_variable sig;
		std::mutex m;
		std::queue<std::variant<uint64_t,std::string>> upload_fifo;
		std::thread upload(http_post_thread,std::ref(upload_fifo),std::ref(m),std::ref(sig));
		
		// Avoid waiting for thread to join if main thread ends
		upload.detach(); 

		std::condition_variable sig_db;
		std::mutex m_db;
		std::queue<profile> db_fifo;
    std::string what = "profile.db";
		std::thread db_store(store_profile_thread,std::ref(db_fifo),std::ref(m_db),std::ref(sig_db),std::ref(what));
		db_store.detach();

		while(recorder_data.resume() && cnt-- > 0) {
      auto [y,x,z] = recorder_data();
      
      auto [left_edge_index,right_edge_index] = find_profile_edges(z,nan_num);
      auto [intercept,gradient] = linear_regression(z);
      gradient /= x_resolution;

      std::transform(z.begin(), z.end(), z.begin(),[gradient, i = 0](int16_t v) mutable -> int16_t{return v != InvalidRange16Bit ? v - (gradient*i++) : InvalidRange16Bit;});


			profile profile{y,x,z};

			profile_buffer.push_back(profile);

			// Wait for buffers to fill
			if (profile_buffer.size() <= 128) continue;

      auto [z_min,z_max] = find_minmax_z(profile_buffer);
      
      spdlog::info("M:{}, m: {},  Thick {}", z_max, z_min, (z_max - z_min ) * z_resolution);
      auto hist = histogram(profile_buffer,z_min,z_max);
      const auto M = get<0>(hist[0]);
      const auto thickness = z_height / z_resolution;
      auto f = hist | views::filter([thickness,M](tuple<double,int16_t> a ){ return M - get<0>(a) > thickness; });
      vector<tuple<double,int16_t>> bot(f.begin(),f.end());
      
      spdlog::info("barrel top: {} bottom : {}", get<0>(hist[0]), get<0>(bot[0]));
      resolution["z_off"] = -1*z_resolution*get<0>(bot[0]);
      std::unique_lock<std::mutex> lock(m);
      upload_fifo.push(resolution.dump());
      lock.unlock();
      sig.notify_one();
    }

    cnt = y_max_length;
    auto yy = 0;
    while(recorder_data.resume() && cnt-- > 0) {
      auto [y,x,z] = recorder_data();
      
      auto [left_edge_index,right_edge_index] = find_profile_edges(z,nan_num);
      auto [intercept,gradient] = linear_regression(z);
      
      std::transform(z.begin(), z.end(), z.begin(),[gradient, i = 0](int16_t v) mutable -> int16_t{return v != InvalidRange16Bit ? v - (gradient*i++) : InvalidRange16Bit;});

			auto f = z | views::take(right_edge_index) | views::drop(left_edge_index);
      profile front_profile{yy++,x,vector<int16_t>(f.begin(),f.end())};

      if(!store_profile(stmt,front_profile)) {
				std::unique_lock<std::mutex> lock(m_db);
				db_fifo.push(front_profile);
				lock.unlock();
				sig_db.notify_one();
			}

      if(upload_fifo.size() < y_max_samples) {
        std::unique_lock<std::mutex> lock(m);
        upload_fifo.push(front_profile.y);
        lock.unlock();
        sig.notify_one();
			}

    }
		
    gocator.Stop();
    close_db(db,stmt,fetch_stmt);
	}

  	void process_flatbuffers()
	{
	  auto db_name = global_config["db_name"].get<std::string>();
    create_db(db_name);
    auto [db, stmt] = open_db(db_name);
    auto fetch_stmt = fetch_profile_statement(db);

    auto data_src = global_config["data_source"].get<std::string>();	
		BlockingReaderWriterQueue<profile> gocatorFifo(4096*1024);
    BlockingReaderWriterQueue<profile> db_fifo;
    BlockingReaderWriterQueue<std::variant<uint64_t,std::string>> upload_fifo;
		
    unique_ptr<GocatorReaderBase> gocator;
    if(data_src == "gocator"s) {
      gocator =  make_unique<GocatorReader>(gocatorFifo);
    }else{
      gocator = make_unique<SqliteGocatorReader>(gocatorFifo);
    }
		gocator->Start();

		// Must be first access to in_file; These values get written once
		auto [y_resolution,x_resolution,z_resolution,z_offset] = gocator->get_gocator_constants();
    store_profile_parameters(y_resolution,x_resolution,z_resolution,z_offset);

		json resolution;
	
		resolution["y"] = y_resolution;
		resolution["x"] = x_resolution;
		resolution["z"] = z_resolution;
		resolution["z_off"] = z_offset;
  	auto fnrows = global_config["fiducial_y"].get<double>();
	  auto fgap = global_config["fiducial_gap"].get<double>();
	  auto fncols = global_config["fiducial_x"].get<double>();
    auto fdepth = global_config["fiducial_depth"].get<double>() / z_resolution;
		
    auto fiducial = make_fiducial(x_resolution,y_resolution,fnrows,fncols,fgap);
    fiducial_as_image(fiducial);

		spdlog::info("Gocator constants {}", resolution.dump());
	
      uint64_t py = 0, cnt = 0;
		auto recorder_data = get_flatworld(std::ref(gocatorFifo));

    const int nan_num = global_config["left_edge_nan"].get<int>();
		const double belt_crosscorr_threshold = global_config["belt_cross_correlation_threshold"].get<double>();
		
    const double z_height = global_config["z_height"].get<double>();
    const auto y_max_length = global_config["y_max_length"].get<uint64_t>();
		uint64_t y_max_samples = (uint64_t)(global_config["y_max_length"].get<double>()/y_resolution);

    const int x_samples = global_config["x_width"].get<double>() / x_resolution;
    
//		std::thread upload(http_post_thread,std::ref(upload_fifo));
		
		// Avoid waiting for thread to join if main thread ends
		//upload.detach(); 

	  //std::thread db_store(store_profile_thread,std::ref(db_fifo));
		
    //db_store.detach();

		deque<profile> profile_buffer;
		bool pending_meta_upload = true;
		uint64_t found_origin_at = 0;
		int16_t cv_threshhold = 0;

    double pmin = numeric_limits<double>::max();
    auto p_now = high_resolution_clock::now();

    int16_t barrel_z = 0;
		
    double intercept = 0.0 ,gradient = 0.0;

    while(recorder_data.resume()) {
      auto [y,x,z] = recorder_data();
  
      z = nan_removal(z,-3000);

      auto [left_edge_index,right_edge_index] = find_profile_edges(z,nan_num,x_samples);
      auto [tintercept,tgradient] = linear_regression(z,left_edge_index,right_edge_index);
      
      intercept = ((intercept != 0.0 ? intercept : tintercept) + tintercept) / 2;
      gradient = ((gradient != 0.0 ? gradient : tgradient)  + tgradient / x_resolution) / 2.0;

      //std::transform(z.begin(), z.end(), z.begin(),[gradient, i = 0](int16_t v) mutable -> int16_t{return v != InvalidRange16Bit ? v - (gradient*i++) : InvalidRange16Bit;});


			profile profile{y,x,z};

			profile_buffer.push_back(profile);

			// Wait for buffers to fill
			if (profile_buffer.size() < fiducial.rows) continue;

      auto [z_min,z_max] = find_minmax_z(profile_buffer);

      spdlog::info("M:{}, m: {},  Thick {}", z_max, z_min, (z_max - z_min ) * z_resolution);
      auto hist = histogram(profile_buffer,z_min,z_max);
      const auto M = get<0>(hist[0]);
      const auto thickness = z_height / z_resolution;
      auto f = hist | views::filter([thickness,M](tuple<double,int16_t> a ){ return M - get<0>(a) > thickness; });
      vector<tuple<double,int16_t>> bot(f.begin(),f.end());
      barrel_z = get<0>(bot[0]);
      spdlog::info("barrel top: {} bottom : {}", get<0>(hist[0]), get<0>(bot[0]));
      resolution["z_off"] = -1*z_resolution*get<0>(bot[0]);
  //    upload_fifo.enqueue(resolution.dump());

      cv_threshhold = get<0>(hist[0]) - (global_config["fiducial_depth"].get<double>() / z_resolution) ;

      break;
    }
   
    uint64_t yy = 0;
    double min_corr = numeric_limits<double>::max() / 1.1;
    cv::Mat min_mat;
    while(recorder_data.resume()) {
      auto [y,x,z] = recorder_data();
      
      z = nan_removal(z,-3000);       
      auto [left_edge_index,right_edge_index] = find_profile_edges(z,nan_num,x_samples);
      
      auto f = z | views::take(right_edge_index) | views::drop(left_edge_index);
      profile profile{yy,x+left_edge_index*x_resolution,vector<int16_t>(f.begin(),f.end())};

     std::transform(profile.z.begin(), profile.z.end(), profile.z.begin(),[gradient, i = 0](int16_t v) mutable -> int16_t{return v != InvalidRange16Bit ? v - (gradient*i++) : InvalidRange16Bit;});
      

      profile_buffer.pop_front();
      profile_buffer.push_back(profile);
      
      auto belt = buffers_to_mat(profile_buffer,x_resolution);
      double minVal, maxVal;
      minMaxLoc( belt.colRange(0,fiducial.cols*2), &minVal, &maxVal);
      cv_threshhold = maxVal - fdepth;
			auto found_origin = samples_contains_fiducial(belt,fiducial,belt_crosscorr_threshold,cv_threshhold);

      if(found_origin < min_corr) {
        min_mat = belt.clone();
      }
      min_corr = std::min(found_origin,min_corr);

      if(found_origin < belt_crosscorr_threshold) {
        spdlog::info("found: {}, thresh: {}, y: {}", found_origin,cv_threshhold,yy);
      				uint64_t i = 0;
			       				 
	// Reset buffer y values to origin
	for(auto &p : profile_buffer) {
  		p.y = i++;
	}	
       mat_as_image(belt.colRange(0,fiducial.cols*2),cv_threshhold);
       fiducial_as_image(belt);
        yy = i;
        break;
      }


      if(yy > y_max_samples) {
        spdlog::info("Looped, Min Corr: {}",min_corr);
        min_corr = numeric_limits<double>::max() / 1.1;
        mat_as_image(min_mat.colRange(0,fiducial.cols*2),cv_threshhold);
        fiducial_as_image(min_mat);
        yy = 0;
      }else {
        yy++;
      }

    }
 

  auto start = std::chrono::high_resolution_clock::now();
  cnt = 0;
    while(recorder_data.resume() && y_max_samples-- > 0) {
      ++cnt;
  
      auto [y,x,z] = recorder_data();

      z = nan_removal(z,-3000);       
      auto [left_edge_index,right_edge_index] = find_profile_edges(z,nan_num,x_samples);
      
      auto f = z | views::take(right_edge_index) | views::drop(left_edge_index);
      profile profile{yy++,x+left_edge_index*x_resolution,vector<int16_t>(f.begin(),f.end())};

      std::transform(profile.z.begin(), profile.z.end(), profile.z.begin(),[gradient, i = 0](int16_t v) mutable -> int16_t{return v != InvalidRange16Bit ? v - (gradient*i++) : InvalidRange16Bit;});
			
			profile_buffer.push_back(profile);

			cads::profile front_profile = profile_buffer.front();
			
			if(!store_profile(stmt,front_profile)) {
	//			db_fifo.enqueue(front_profile);
			}

//			upload_fifo.enqueue(front_profile.y);
     

			profile_buffer.pop_front();

		}

    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
    spdlog::info("CADS - CNT: {}, DUR: {}, RATE(ms):{} ",cnt, duration, (double)cnt / duration);

		gocator->Stop();
    spdlog::info("Gocator Stopped");
//		db_store.join();
//		upload.join();
	//	close_db(db,stmt,fetch_stmt);
	}

#endif

}
