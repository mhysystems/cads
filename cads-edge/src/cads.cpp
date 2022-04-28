#include <opencv2/core.hpp>
#include <opencv2/core/types.hpp>
#include <opencv2/imgproc.hpp>

#include <csv.hpp>
#include <cads.h>
#include <regression.h>
#include <json.hpp>
#include <db.h>
#include <upload.h>
#include <constants.h>
#include <fiducial.h>
#include <nan_removal.h>
#include <window.hpp>
#include <readerwriterqueue.h>
#include <gocator_reader.h>
#include <sqlite_gocator_reader.h>
#include <generator.hpp>

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

#include <spdlog/spdlog.h>
#include <spdlog/sinks/rotating_file_sink.h>

#include <filters.h>
#include <edge_detection.h>


using namespace std;
using namespace moodycamel;
using json = nlohmann::json;
using namespace std::chrono;
using CadsMat = cv::UMat; //cv::cuda::GpuMat

extern json global_config;

bool trace = false;

auto glog = spdlog::rotating_logger_st("cads", "cads.log", 1024 * 1024 * 5, 1);

namespace cads
{


  tuple<z_element,z_element> barrel_offset(int cnt, double z_resolution, generator<gocator_profile>& ps) {

    window win;
    const auto z_height_mm = global_config["z_height"].get<double>();

    while(ps.resume() && cnt-- > 0) {
      auto [f,x,z] = ps();

      win.push_back({f,x,z});
    }
     
    auto [z_min,z_max] = find_minmax_z(win);
    
    // Histogram, first is z value, second is count 
    auto hist = histogram(win,z_min,z_max);
    
    const auto peak = get<0>(hist[0]);
    const auto thickness = z_height_mm / z_resolution;
    
    // Remove z values greater than the peak minus approx belt thickness.
    // Assumes the next peak will be the barrel values
    auto f = hist | views::filter([thickness,peak](tuple<double,z_element> a ){ return peak - get<0>(a) > thickness; });
    vector<tuple<double,z_element>> barrel(f.begin(),f.end());
    
    return {get<0>(barrel[0]),peak};

  }


  double belt_regression(int cnt, generator<gocator_profile>& ps) {

    const int nan_num = global_config["left_edge_nan"].get<int>();
    const auto z_height_mm = global_config["z_height"].get<double>();
    const int spike_window_size =  nan_num / 3;
    double gradient = 0.0, intercept = 0.0;
    const double n = cnt - 1;

    while(ps.resume() && cnt-- > 0) {
      auto [f,x,z] = ps();

      spike_filter(z,spike_window_size);
      auto [left_edge_index,right_edge_index] = find_profile_edges_nans_outer(z,nan_num);
      nan_filter(z);
      auto [i,g] = linear_regression(z,left_edge_index,right_edge_index);
      intercept += i;
      gradient += g;
    }
     
    return gradient / n;

  }

 std::tuple<window,uint64_t> find_first_origin(const cv::Mat& fiducial, double y_resolution, double x_resolution, double z_resolution, double gradient, z_element bottom, generator<gocator_profile>& recorder_data) {

    const int nan_num = global_config["left_edge_nan"].get<int>();
		const double belt_crosscorr_threshold = global_config["belt_cross_correlation_threshold"].get<double>();
    const auto fdepth = global_config["fiducial_depth"].get<double>() / z_resolution;
    const int spike_window_size =  nan_num / 3;
    
    window profile_buffer;

    uint64_t frame_offset = 0;
    while(recorder_data.resume()) {
      auto [y,x,z] = recorder_data();

      spike_filter(z,spike_window_size);
      auto [left_edge_index,right_edge_index] = find_profile_edges_nans_outer(z,nan_num);
      nan_filter(z);
      regression_compensate(z,left_edge_index,right_edge_index, gradient);
      barrel_height_compensate(z,bottom - z[left_edge_index - 1]);
      
      auto f = z | views::take(right_edge_index) | views::drop(left_edge_index);
      profile profile{y,x+left_edge_index*x_resolution,{f.begin(),f.end()}};

      profile_buffer.push_back(profile);
			// Wait for buffers to fill
			if (profile_buffer.size() <= fiducial.rows) continue;
      profile_buffer.pop_front();

     
      auto belt = window_to_mat(profile_buffer,x_resolution);
      const auto le = left_edge_avg_height(belt,fiducial);
      const auto cv_threshhold = le - fdepth;
			auto correlation = search_for_fiducial(belt,fiducial,cv_threshhold);

      if(correlation < belt_crosscorr_threshold) {
  			uint64_t i = 0;
        frame_offset = y;
			  // Reset buffer y values to origin
        for(auto &p : profile_buffer) {
          p.y = i++;
        }

        break;
      }
    }

    return {profile_buffer,frame_offset - profile_buffer.size() - 1};
  }

	void process_flatbuffers()
	{
	
	}


void store_profile_only()
{

  auto db_name = global_config["db_name"].get<std::string>();
  create_db(db_name);

  auto data_src = global_config["data_source"].get<std::string>();
  BlockingReaderWriterQueue<profile> gocatorFifo;
  
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
  auto [db, stmt] = open_db(db_name);

  auto recorder_data = get_flatworld(std::ref(gocatorFifo));

  uint64_t y_max_samples = (uint64_t)(global_config["y_max_length"].get<double>()/y_resolution);
  auto start = std::chrono::high_resolution_clock::now();
  
  while(recorder_data.resume() && y_max_samples-- > 0) {
    auto [y,x,z] = recorder_data();

    profile profile{y,x,z};
    store_profile(stmt,profile);

  }

  gocator->Stop();
  close_db(db,stmt);

}


	void process_experiment()
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
		auto db_t = store_profile_thread(db_fifo);
    //db_store.detach();

		

		z_element cv_threshhold = 0;
    const int spike_window_size =  nan_num / 4;

    auto [bottom, top] = barrel_offset(1024,z_resolution,recorder_data);
    spdlog::info("Belt Avg - top: {} bottom : {}, height(mm) : {}", top, bottom, (top - bottom) / z_resolution);
    auto gradient = belt_regression(64,recorder_data);
    auto [profile_buffer, frame_offset] = find_first_origin(fiducial, y_resolution, x_resolution, z_resolution, gradient, bottom, recorder_data);
        
     auto start = std::chrono::high_resolution_clock::now();
    
    while(recorder_data.resume()) {
      auto [y,x,z] = recorder_data();

      spike_filter(z,spike_window_size);
      auto [left_edge_index,right_edge_index] = find_profile_edges_nans_outer(z,nan_num);
      nan_filter(z);
      regression_compensate(z,left_edge_index,right_edge_index,gradient);
      barrel_height_compensate(z,bottom - z[left_edge_index - 1]);

      auto f = z | views::take(right_edge_index) | views::drop(left_edge_index);
      profile profile_back{y - frame_offset,x+left_edge_index*x_resolution,{f.begin(),f.end()}};

      profile_buffer.push_back(profile_back);
      profile profile = profile_buffer.front();
      profile_buffer.pop_front();

      db_fifo.enqueue(profile);
      
      if(profile.y > y_max_samples * 0.95) {
        auto belt = window_to_mat(profile_buffer,x_resolution);
        const auto cv_threshhold = left_edge_avg_height(belt,fiducial) - fdepth;
        auto correlation = search_for_fiducial(belt,fiducial,cv_threshhold);

        if(correlation < belt_crosscorr_threshold) {
          break;
        }
      }

    }
 
    db_fifo.enqueue({std::numeric_limits<uint64_t>::max(), NAN, {}});

    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
    spdlog::info("CADS - CNT: {}, DUR: {}, RATE(ms):{} ",cnt, duration, (double)cnt / duration);

		gocator->Stop();
    spdlog::info("Gocator Stopped");

		close_db(db,stmt,fetch_stmt);
    db_t.join();
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


