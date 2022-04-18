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

#include <readerwriterqueue.h>
#include <gocator_reader.h>
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

	std::string slurpfile(const std::string_view path, bool binaryMode)
	{
		std::ios::openmode openmode = std::ios::in;
		if (binaryMode)
		{
			openmode |= std::ios::binary;
		}
		std::ifstream ifs(path.data(), openmode);
		ifs.ignore(std::numeric_limits<std::streamsize>::max());
		std::string data(ifs.gcount(), 0);
		ifs.seekg(0);
		ifs.read(data.data(), data.size());
		return data;
	}


template<typename T> int edge(T s, T e,int len);


template<typename T> int belt(T s, T e,int len) {

  namespace sr = std::ranges;

  auto r = sr::find_if(s,e,[](int16_t x) {return x == InvalidRange16Bit;} );
  auto d = sr::distance(s,r);
  if(r == e) return d;
  else return d + edge(r,e,len);

}


template<typename T> int edge(T s, T e,int len) {

  namespace sr = std::ranges;
  auto r = sr::find_if(s,e,[](int16_t x) {return x != InvalidRange16Bit;} );
  auto d = sr::distance(s,r);

  if(d >= len || r == e) {
    return 0;
  }
  else{
    auto window_len = s + len > e ? sr::distance(s,e) : len;
    auto nan_count = (double)sr::count(s,s + window_len,InvalidRange16Bit);
    if(nan_count / window_len > 0.9) return 0;
    else return d + belt(r,e,len);
  }

}

auto find_profile_edges(std::vector<int16_t> z, int len) {

  auto mid = int(z.size() / 2);
  auto r = mid + belt(z.begin()+mid,z.end(),len) - 1;
  auto l = z.size() - mid - belt(z.rbegin() + mid ,z.rend(),len);
  return std::tuple<int,int>{l, r};
}

// Edge can be detected by a sequence on NaN on either side of the belt.
// The number of Nan is fixed and is dependent by the configuration of the
// laser and camera and belt thickness.
// e_x0 & e_x1 are the estimated positions of the edges

auto find_belt_edges(profile p, int len, double x_res) {

	const int nCols = p.z.size();
	const int s = 1;
	const int e_x0 = nCols / 2;
	const int e_x1 = e_x0;

	int min_x = std::numeric_limits<int>::max();
	int max_x = std::numeric_limits<int>::min();
	int nan_count = 0;
	
	int j = 0;
	int k = 0;
	for (j = e_x0 + len*s; j >=0 ;--j)
	{	
		for(k = 0,nan_count = 0;((j-k) >= 0) && InvalidRange16Bit == p.z[j-k];++k) nan_count++;
		
		if((j-k) < 0 || (nan_count >= len /*&& (p[j+1] - p[j-k] > 10)*/)) break;
			
		j -= k;
	}

	min_x = std::min(min_x,j+1);

	for (j = e_x1 - len*s; j < nCols ; ++j)
	{	
		for(k=0,nan_count =0;((j+k) < nCols) && InvalidRange16Bit == p.z[j+k];++k) nan_count++;
		
		if((j+k) >= nCols || (nan_count >= len /*&& (p[j-1] - p[j+k] > 10)*/)) break;
		j += k;
	}
	
	max_x = std::max(max_x,j-1);
  	
	return std::tuple<double,double>({p.x_off + x_res*min_x, p.x_off + x_res*max_x});

}


// Edge can be detected by a sequence on NaN on either side of the belt.
// The number of Nan is fixed and is dependent by the configuration of the
// laser and camera and belt thickness.
// e_x0 & e_x1 are the estimated positions of the edges

auto find_profile_edges2(std::vector<int16_t> z, int len) {

	const int nCols = z.size();
	const int s = 1;
	const int e_x0 = nCols / 2;
	const int e_x1 = e_x0;

	int min_x = std::numeric_limits<int>::max();
	int max_x = std::numeric_limits<int>::min();
	int nan_count = 0;
	
	int j = 0;
	int k = 0;
	for (j = e_x0 + len*s; j >=0 ;--j)
	{	
		for(k = 0,nan_count = 0;((j-k) >= 0) && InvalidRange16Bit == z[j-k];++k) nan_count++;
		
		if((j-k) < 0 || (nan_count >= len /*&& (p[j+1] - p[j-k] > 10)*/)) break;
			
		j -= k;
	}

	min_x = std::min(min_x,j+1);

	for (j = e_x1 - len*s; j < nCols ; ++j)
	{	
		for(k=0,nan_count =0;((j+k) < nCols) && InvalidRange16Bit == z[j+k];++k) nan_count++;
		
		if((j+k) >= nCols || (nan_count >= len /*&& (p[j-1] - p[j+k] > 10)*/)) break;
		j += k;
	}
	
	max_x = std::max(max_x,j-1);
  	
  return std::tuple<int,int>{min_x, max_x};

}


double samples_contains_fiducial(cv::Mat belt, cv::Mat fiducial, double c_threshold, double z_threshold) {
	
	cv::Mat black_belt;
	cv::threshold(belt.colRange(0,fiducial.cols*4),black_belt,z_threshold,1.0,cv::THRESH_BINARY);

	cv::Mat out(black_belt.rows - fiducial.rows + 1,black_belt.cols - fiducial.cols + 1, CV_32F);
	cv::matchTemplate(black_belt,fiducial,out,cv::TM_SQDIFF_NORMED);

	double minVal; double maxVal; cv::Point minLoc; cv::Point maxLoc;
  minMaxLoc( out, &minVal, &maxVal, &minLoc, &maxLoc, cv::Mat() );
	
	auto r = std::abs(minVal) < c_threshold ? true : false;

	return minVal;

}

double samples_contains_fiducial_gpu(CadsMat belt, CadsMat fiducial, double c_threshold, double z_threshold) {
	
	CadsMat black_belt; //.colRange(0,fiducial.cols*2)
	cv::threshold(belt.colRange(0,fiducial.cols*2),black_belt,z_threshold,1.0,cv::THRESH_BINARY);

	CadsMat out(black_belt.rows - fiducial.rows + 1,black_belt.cols - fiducial.cols + 1, CV_32F);
	cv::matchTemplate(black_belt,fiducial,out,cv::TM_SQDIFF_NORMED);

	double minVal; double maxVal; cv::Point minLoc; cv::Point maxLoc;
  minMaxLoc( out, &minVal, &maxVal, &minLoc, &maxLoc, cv::Mat() );
	
	return minVal;

}


bool compare_samples(profile a, profile b, int threshold = 100) {
	auto az = a.z;
	auto bz = b.z;
	auto len = min(az.size(),bz.size());
	
	int cnt = 0;
	
	// 40 was found to be the variance between z samples of a non moving scan
	for(size_t i = 0; i < len; i++) {
		if(abs(az[i] - bz[i]) > 40) cnt++;
	}

	return cnt < threshold && cnt > len*0.5;
}


tuple<double,double>  x_minmax(profile_window win, double x_res) {
  double x_min = numeric_limits<double>::max();
  double x_max = numeric_limits<double>::min();
  
  for(auto p : win) {
    x_min = min(x_min,p.x_off);
    x_max = max(x_max,p.x_off+p.z.size()*x_res);
  }

  return {x_min,x_max};
}

cv::Mat buffers_to_mat(profile_window ps, double x_res) {

  if(ps.size() < 1) return cv::Mat(0,0,CV_32F);

	
	auto [x_min,x_max] = x_minmax(ps,x_res);

	const int n_cols = ((x_max-x_min)/x_res);
  const int n_rows = ps.size();
	cv::Mat mat(n_rows,n_cols,CV_32F,cv::Scalar::all(0.0f));
	
  int i = 0;
  for(auto p : ps) {

    auto m = mat.ptr<float>(i++);

    int j = (p.x_off - x_min) / x_res;
    
    for(auto z : p.z) {
       m[j++] = (float)z;
    }


  }
	return mat;
}

#if 0
CadsMat buffers_to_mat_gpu(profile_window ps, double x_res) {

  if(ps.size() < 1) return CadsMat(0,0,CV_32F);

	
	const auto pxminmax = reduce(ps.begin(),ps.end(),profile{0,0.0,numeric_limits<double>::max(),numeric_limits<double>::min(), std::vector<int16_t>()},[x_res](auto acc, auto x) {
		return profile{acc.y,acc.x_off,min(acc.left_edge,x.left_edge),max(acc.right_edge,x.right_edge),acc.z};
	});

  const auto x_min = pxminmax.left_edge;
  const auto x_max = pxminmax.right_edge;
	const auto n_cols = int((x_max-x_min)/x_res);
	cv::Mat mat(ps.size(),n_cols,CV_32F,cv::Scalar::all(0.0f));
	
  int i = 0;
  for(auto p : ps) {

    auto p_x_min = p.left_edge;
    auto p_x_max = p.right_edge;

    auto m = mat.ptr<float>(i++);
    const auto zeros_left = (int)((p_x_min - x_min)/x_res); // Should be a multiple of x_res

    int j = 0;
    for(;j <zeros_left;++j) {
            m[j] = 0.0f;
    }

    auto x_begin = int(p_x_min / x_res);
    auto x_end = int(p_x_max / x_res);

    for_each(p.z.begin() + x_begin ,p.z.begin() + x_end,[&](auto z) {
            m[j++] = (float)z;
    });

    for(;j < n_cols;++j) {
            m[j] = 0.0f;
    }


  }
	return mat.getUMat(cv::ACCESS_READ,cv::USAGE_ALLOCATE_DEVICE_MEMORY);
}




generator<gocator_profile> get_flatworld(std::istream& in_file) {

	while(in_file) {
		int size = 0;
		in_file.read((char*)&size,sizeof(size));

		if(!in_file || size < 1) {
			break;
		} 

		auto buf = std::make_unique<char[]>(size);
		// Assume all bytes are read in one call
		in_file.read(buf.get(),size);

		co_yield extractZData(std::move(buf));
	}
}




auto get_gocator_constants(std::istream& in_file) {
	
	int size = 0;
	in_file.read((char*)&size,sizeof(size));

	auto buf = std::make_unique<char[]>(size);
	// Assume all bytes are read in one call
	in_file.read(buf.get(),size);

	return getProfile(std::move(buf));

}


cv::Mat slurpcsv_mat(std::string filename)
	{
		using namespace csv;
		const int skip_columns = 0;
		cv::Mat out;
	
		CSVFormat f;
		f.no_header();

		CSVReader reader(filename, f);

		for (CSVRow &row : reader)
		{

			int x = 0, xx = 0;
			auto width = row.size() - skip_columns;

			cv::Mat t(1, width, CV_32F);

			for (CSVField &field : row)
			{

				if (x < skip_columns)
				{
					x++;
					continue;
				}
				if (x++ > skip_columns + width)
					break;

				std::string g = field.get<std::string>();
				t.at<float>(0, xx++) = g != "" ? std::stof(g) : NAN;
			}
			out.push_back(t);
		}

		if (trace)
			std::cerr << std::endl;

		return out;
	}
#endif
		vector<tuple<double,int16_t>> histogram(profile_window ps, int16_t min, int16_t max, double size = 100) {
		
    const auto dz = (size-1) / (max - min);
    vector<tuple<double,int16_t>> hist(size,{0,0}); 
		
		for(auto p : ps) {
			auto zs = p.z;
			for(auto z: zs) {
				if(z != InvalidRange16Bit) {
					int i = (z - min)*dz;
					hist[i] = {(i + 2)*(1/dz)+min,1+get<1>(hist[i])};
				}
			}
		}
		
    ranges::sort(hist,[](auto a, auto b){ return get<1>(a) > get<1>(b);});
    return hist;
	}


	tuple<int16_t,int16_t> find_minmax_z(profile_window ps) {
		auto z_min = std::numeric_limits<int16_t>::max();
		auto z_max = std::numeric_limits<int16_t>::min();
		for(auto p : ps) {
			auto zs = p.z;
			for(auto z: zs) {
				if(z != InvalidRange16Bit) {
					z_min = std::min(z,z_min);
					z_max = std::max(z,z_max);
				}
			}
		}

		return {z_min,z_max};
	}


	void process_flatbuffers()
	{
				
		BlockingReaderWriterQueue<char> gocatorFifo;
    BlockingReaderWriterQueue<profile> db_fifo;
    BlockingReaderWriterQueue<std::variant<uint64_t,std::string>> upload_fifo;
		
    unique_ptr<GocatorReaderBase> gocator = make_unique<GocatorReader>(gocatorFifo);
		gocator->Start();

		// Must be first access to in_file; These values get written once
		auto [y_resolution,x_resolution,z_resolution,z_offset] = get_gocator_constants(std::ref(gocatorFifo));

		json resolution;
	
		resolution["y"] = y_resolution;
		resolution["x"] = x_resolution;
		resolution["z"] = z_resolution;
		resolution["z_off"] = z_offset;
  	auto fnrows = global_config["fiducial_y"].get<double>();
	  auto fgap = global_config["fiducial_gap"].get<double>();
	  auto fncols = global_config["fiducial_x"].get<double>();
    auto fdepth = global_config["fiducial_depth"].get<double>() / z_resolution;
    auto db_name = global_config["db_name"].get<std::string>();
		
    auto fiducial = make_fiducial(x_resolution,y_resolution,fnrows,fncols,fgap);
    fiducial_as_image(fiducial);

		spdlog::info("Gocator constants {}", resolution.dump());
			
		auto recorder_data = get_flatworld(std::ref(gocatorFifo));
		
    const int nan_num = global_config["left_edge_nan"].get<int>();
		const double belt_crosscorr_threshold = global_config["belt_cross_correlation_threshold"].get<double>();
		
    const double z_height = global_config["z_height"].get<double>();
    const auto y_max_length = global_config["y_max_length"].get<uint64_t>();
		const uint64_t y_max_samples = (uint64_t)(global_config["y_max_length"].get<double>()/y_resolution);
    
    create_db(db_name);
		auto [db, stmt] = open_db(db_name);
		auto fetch_stmt = fetch_profile_statement(db);

		std::thread upload(http_post_thread,std::ref(upload_fifo));
		
		// Avoid waiting for thread to join if main thread ends
		upload.detach(); 

	  std::thread db_store(store_profile_thread,std::ref(db_fifo));
		
    db_store.detach();

		deque<profile> profile_buffer;
		bool pending_meta_upload = true;
		uint64_t found_origin_at = 0;
		int16_t cv_threshhold = 0;
    uint64_t py = 0, cnt = 0;
    double pmin = numeric_limits<double>::max();
    auto p_now = high_resolution_clock::now();

    int16_t barrel_z = 0;
		
    while(recorder_data.resume()) {
      auto [y,x,z] = recorder_data();
      
      auto [left_edge_index,right_edge_index] = find_profile_edges(z,nan_num);
      auto [intercept,gradient] = linear_regression(z,left_edge_index,right_edge_index);
      gradient /= x_resolution;

      std::transform(z.begin(), z.end(), z.begin(),[gradient, i = 0](int16_t v) mutable -> int16_t{return v != InvalidRange16Bit ? v - (gradient*i++) : InvalidRange16Bit;});


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
      //resolution["z_off"] = -1*z_resolution*get<0>(bot[0]);
      cv_threshhold = get<0>(hist[0]) - (global_config["fiducial_depth"].get<double>() / z_resolution) ;

      break;
    }
   
    uint64_t yy = 0;
    double min_corr = numeric_limits<double>::max();

    while(recorder_data.resume()) {
      auto [y,x,z] = recorder_data();
      
      auto [left_edge_index,right_edge_index] = find_profile_edges(z,nan_num);
      z = nan_removal(z,barrel_z); 
      auto [intercept,gradient] = linear_regression(z,left_edge_index,right_edge_index);
      
      std::transform(z.begin(), z.end(), z.begin(),[gradient, i = 0](int16_t v) mutable -> int16_t{return v != InvalidRange16Bit ? v - (gradient*i++) : InvalidRange16Bit;});

			auto f = z | views::take(right_edge_index) | views::drop(left_edge_index);
      profile profile{yy++,x,vector<int16_t>(f.begin(),f.end())};

      profile_buffer.pop_front();
      profile_buffer.push_back(profile);
      
      if(!store_profile(stmt,profile)) {
				db_fifo.enqueue(profile);
			}
      

      auto belt = buffers_to_mat(profile_buffer,x_resolution);
      double minVal, maxVal;
      minMaxLoc( belt.colRange(0,fiducial.cols*2), &minVal, &maxVal);
      cv_threshhold = maxVal - fdepth;
			auto found_origin = samples_contains_fiducial(belt,fiducial,belt_crosscorr_threshold,cv_threshhold);

      if(found_origin < min_corr * belt_crosscorr_threshold && found_origin < 0.3) {
        spdlog::info("found: {}, thresh: {}, y: {}", found_origin,cv_threshhold,yy);
        min_corr = std::min(found_origin,min_corr);
        mat_as_image(belt.colRange(0,fiducial.cols*2),cv_threshhold);
        fiducial_as_image(belt);
        yy = 0;
      }

      if(yy > y_max_samples) {
        spdlog::info("Looped");
        yy = 0;
      }

      yy++;

    }

    while(recorder_data.resume()) {
      auto now = std::chrono::high_resolution_clock::now();
			auto seconds = std::chrono::duration_cast<std::chrono::seconds>(now - p_now);
      p_now = now;

			auto [y,x,z] = recorder_data();
      auto [left_edge_index,right_edge_index] = find_profile_edges(z,nan_num);
      auto [intercept,gradient] = linear_regression(z,left_edge_index,right_edge_index);
      gradient /= x_resolution;

      std::transform(z.begin(), z.end(), z.begin(),[gradient, i = 0](int16_t v) mutable -> int16_t{return v != InvalidRange16Bit ? v - (gradient*i++) : InvalidRange16Bit;});

			
      if(y > y_max_samples) {
        glog->info("Max Y samples reached y:{} max:{}",y,y_max_samples);
      }


      if(y == 0) found_origin_at = 0;
			y = (y - found_origin_at ) % y_max_samples;

      if(y - py > 1) {
        glog->info("Skipped y:{} py:{}", y, py);
      }
      py = y;
			profile profile{y,x,z};

			profile_buffer.push_back(profile);

			// Wait for buffers to fill
			if (profile_buffer.size() < fiducial.rows) continue;

	
			if(cnt == 0 || cnt > 2000) {
				pending_meta_upload = false;
				auto [z_min,z_max] = find_minmax_z(profile_buffer);
        spdlog::debug("Zmin:{} Zmax:{}", z_min, z_max);

				resolution["z_off"] = -1*z_resolution*z_min;

				auto hist = histogram(profile_buffer,z_min,z_max);
        spdlog::debug("belt height: {}, width:{}, l:{}, r:{}",z_resolution * get<0>(hist[0]),(right_edge_index - left_edge_index)*x_resolution,x+x_resolution*left_edge_index,x+x_resolution*right_edge_index);
				cv_threshhold = get<0>(hist[0]) - global_config["fiducial_depth"].get<double>()/z_resolution;
        cnt = 0;
			}
      cnt++;

			auto belt = buffers_to_mat(profile_buffer,x_resolution);
			auto found_origin = samples_contains_fiducial(belt,fiducial,belt_crosscorr_threshold,cv_threshhold);

      if((found_origin / pmin) < 1.1) {
        spdlog::info("found: {}, pmin: {}, y: {}", found_origin,pmin,y);
        if(found_origin < pmin) pmin = found_origin;
      }
      
			if(found_origin) {
				found_origin_at = y;
				uint64_t i = 0;
				 
				// Reset buffer y values to origin
				for(auto &p : profile_buffer) {
					p.y = i++;
				}
				glog->info("Fiducial found at y:{}", y);
			}

			cads::profile front_profile = profile_buffer.front();
			
			const auto previous_profile = fetch_profile(fetch_stmt,front_profile.y);
			
			if(!store_profile(stmt,front_profile)) {
				db_fifo.enqueue(front_profile);
			}

  
			auto is_same = compare_samples(front_profile,previous_profile);
			if(!is_same) {
					upload_fifo.enqueue(front_profile.y);
			}
      

			profile_buffer.pop_front();

		}

		gocator->Stop();
		close_db(db,stmt,fetch_stmt);
	}


void store_profile_only()
	{
				
		BlockingReaderWriterQueue<char> gocatorFifo;
		unique_ptr<GocatorReaderBase> gocator = make_unique<GocatorReader>(gocatorFifo);
		gocator->Start();

		// Must be first access to in_file; These values get written once
		auto [y_resolution,x_resolution,z_resolution,z_offset] = get_gocator_constants(std::ref(gocatorFifo));

		json resolution;
	
		resolution["y"] = y_resolution;
		resolution["x"] = x_resolution;
		resolution["z"] = z_resolution;
		resolution["z_off"] = z_offset;
		
		spdlog::info("Gocator constants {}", resolution.dump());
    auto recorder_data = get_flatworld(std::ref(gocatorFifo));
		
		uint64_t y_max_samples = (uint64_t)(global_config["y_max_length"].get<double>()/y_resolution);
    
		auto [db, stmt] = open_db("");
		auto fetch_stmt = fetch_profile_statement(db);
		
  	while(recorder_data.resume() && y_max_samples-- > 0) {


			auto [y,x,z] = recorder_data();

			profile profile{y,x,z};

			
			store_profile(stmt,profile);

		}

		gocator->Stop();
  
    spdlog::info("Gocator Stopped");
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

#endif

}


