#include <opencv2/core.hpp>
#include <opencv2/core/types.hpp>
#include <opencv2/imgproc.hpp>

#include <csv.hpp>
#include <cads.h>
#include <json.hpp>
#include <db.h>
#include <upload.h>

#include <readerwriterqueue.h>
#include <gocator_reader.h>

#include <z_data_generated.h>
#include <p_config_generated.h>

#include <exception>
#include <fstream>
//#include <cmath>
//#include <cstddef>
#include <limits>
//#include <sstream>
#include <string>
#include <vector>
//#include <cstdlib>
#include <tuple>
#include <algorithm>
//#include <valarray>
//#include <memory>
#include <coroutine>
#include <queue>
#include <deque>
//#include <string_view>
#include <thread>
#include <condition_variable>
#include <mutex>
#include <ranges>

#include <spdlog/spdlog.h>
#include <spdlog/sinks/rotating_file_sink.h>


using namespace std;
using namespace moodycamel;
using json = nlohmann::json;

extern json global_config;

bool trace = false;

namespace cads
{
	template <typename T, typename R = int>
	struct generator
	{
		struct promise_type;
		using handle_type = std::coroutine_handle<promise_type>;

		struct suspend_always2
		{
			R &value_out;
			suspend_always2() = default;
			suspend_always2(R &a) : value_out(a) {}
			constexpr bool await_ready() const noexcept { return false; }

			constexpr void await_suspend(handle_type) const noexcept {}

			R await_resume() const noexcept
			{
				return value_out;
			}
		};

		struct promise_type
		{
			T value_in;
			R value_out;
			std::exception_ptr exception_;

			generator get_return_object()
			{
				return generator(handle_type::from_promise(*this));
			}

			std::suspend_always initial_suspend() noexcept { return {}; }
			std::suspend_always final_suspend() noexcept { return {}; }
			void unhandled_exception() { exception_ = std::current_exception(); }

			suspend_always2 yield_value(T from)
			{
				value_in = from;
				return {value_out};
			}
			void return_void() {}
		};

		handle_type coro;

		generator(handle_type h) : coro(h) {}
		~generator() { coro.destroy(); }

		bool resume()
		{
			return coro ? (coro.resume(), !coro.done()) : false;
		}

		bool resume(R a)
		{
			coro.promise().value_out = a;
			return coro ? (coro.resume(), !coro.done()) : false;
		}

		T operator()(R a)
		{
			coro.promise().value_out = a;
			return coro.promise().value_in;
		}

		T operator()()
		{
			return coro.promise().value_in;
		}
	};

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


bool samples_contains_fiducial(cv::Mat belt, cv::Mat fiducial, double c_threshold, double z_threshold) {
	
	cv::Mat black_belt;
	cv::threshold(belt,black_belt,z_threshold,1.0,cv::THRESH_BINARY);

	cv::Mat out(belt.rows - fiducial.rows + 1,belt.cols - fiducial.cols + 1, CV_32F);
	cv::matchTemplate(black_belt,fiducial,out,cv::TM_SQDIFF_NORMED);

	double minVal; double maxVal; cv::Point minLoc; cv::Point maxLoc;
  minMaxLoc( out, &minVal, &maxVal, &minLoc, &maxLoc, cv::Mat() );
	
	auto r = std::abs(minVal) < c_threshold ? true : false;

	return r;

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

cv::Mat buffers_to_mat(deque<profile> ps, deque<tuple<double,double>> boundary, double x_res) {

	if(ps.size() < 1) return cv::Mat(0,0,CV_32F);

	
	const auto [x_min,x_max] = reduce(boundary.begin(),boundary.end(),tuple<double,double>{numeric_limits<double>::max(),numeric_limits<double>::min()},[](auto acc, auto x) {
		return tuple<double,double>{min(get<0>(acc),get<0>(x)),max(get<1>(acc),get<1>(x))};
	});

	const auto n_cols = ((x_max-x_min)/x_res);
	cv::Mat mat(ps.size(),(int)n_cols,CV_32F,cv::Scalar::all(0.0f));
	
  int i = 0;
  auto bounds = boundary.begin();
  for(auto p : ps) {
    auto p_x_min = get<0>(*bounds);
    auto p_x_max = get<1>(*bounds++);

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
	return mat;
}

cv::Mat draw_fiducial(double x_res, double y_res) {

	int nrows = global_config["fiducial_y"].get<double>() / y_res;
	int gap = global_config["fiducial_gap"].get<double>() / y_res;
	int ncols = global_config["fiducial_x"].get<double>() / x_res;

	cv::Mat draw(gap + 2*nrows+5,ncols+2,CV_32F,{1.0});

	cv::rectangle(draw,{2,2,ncols,nrows},{0.0},-1);
	cv::rectangle(draw,{2,2+nrows+gap,ncols,nrows},{0.0},-1);

	return draw;

}



profile extractZData(std::unique_ptr<char[]> buf) {

	using namespace flatbuffers;

	auto profile = cads_flatworld::Getprofile(buf.get());
	auto y = profile->frame();
	auto x_off = profile->x_off();
	auto z_samples = profile->z_samples();
	std::vector<int16_t> z(z_samples->begin(),z_samples->end());

	return {y,x_off,z};

}


generator<profile> get_flatworld(std::istream& in_file) {

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

generator<profile> get_flatworld(BlockingReaderWriterQueue<char>& fifo) {

	while(true) {
		int size = 0;
		
		fifo.wait_dequeue((char*)&size,sizeof(size));

		if(size < 1) {
			break;
		} 

		auto buf = std::make_unique<char[]>(size);
		fifo.wait_dequeue(buf.get(),size);

		co_yield extractZData(std::move(buf));
	}
}


auto getProfile(std::unique_ptr<char[]> buf) {
	
	using namespace flatbuffers;
	auto profile = cads_flatworld::Getprofile_resolution(buf.get());
	auto y_res= profile->y();
	auto x_res = profile->x();
	auto z_res = profile->z();
	auto z_off = profile->z_off();

	return tuple<decltype(y_res),decltype(x_res),decltype(z_res),decltype(z_off)>{y_res,x_res,z_res,z_off};
}


auto get_gocator_constants(std::istream& in_file) {
	
	int size = 0;
	in_file.read((char*)&size,sizeof(size));

	auto buf = std::make_unique<char[]>(size);
	// Assume all bytes are read in one call
	in_file.read(buf.get(),size);

	return getProfile(std::move(buf));

}

auto get_gocator_constants(BlockingReaderWriterQueue<char>& fifo) {
	
	int size = 0;
	
	fifo.wait_dequeue((char*)&size,sizeof(size));

	auto buf = std::make_unique<char[]>(size);
	
	fifo.wait_dequeue(buf.get(),size);

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

	vector<tuple<double,int16_t>> histogram(profile_window ps, int16_t min, int16_t max, double size = 100) {
		
		vector<tuple<double,int16_t>> hist(size,{0,0});
		double dz = (size-1) / (max - min);
		
		for(auto p : ps) {
			auto zs = p.z;
			for(auto z: zs) {
				if(z != InvalidRange16Bit) {
					int i = (z - min)*dz;
					hist[i] = {(i + 0.5)*(1/dz)+min,1+get<1>(hist[i])};
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
		auto log = spdlog::rotating_logger_st("cads", "cads.log", 1024 * 1024 * 5, 1);

		
		BlockingReaderWriterQueue<char> gocatorFifo;
		GocatorReader gocator(gocatorFifo);
		gocator.Start();

		// Must be first access to in_file; These values get written once
		auto [y_resolution,x_resolution,z_resolution,z_offset] = get_gocator_constants(std::ref(gocatorFifo));

		auto fiducial = draw_fiducial(x_resolution,y_resolution);


		json resolution;
	
		resolution["y"] = y_resolution;
		resolution["x"] = x_resolution;
		resolution["z"] = z_resolution;
		resolution["z_off"] = z_offset;
		
		spdlog::info("Gocator constants {}", resolution.dump());
			
		auto recorder_data = get_flatworld(std::ref(gocatorFifo));
		const int nan_num = global_config["left_edge_nan"].get<int>();
		const double belt_crosscorr_threshold = global_config["belt_cross_correlation_threshold"].get<double>();
		
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
		std::thread db_store(store_profile_thread,std::ref(db_fifo),std::ref(m_db),std::ref(sig_db));
		db_store.detach();

		deque<std::tuple<double,double>> belt_boundary_buffer;
		deque<profile> profile_buffer;
		bool pending_meta_upload = true;
		uint64_t found_origin_at = 0;
		int16_t cv_threshhold = 0;


		while(recorder_data.resume()) {
			log->flush();

			auto [y,x,z] = recorder_data();

			if(y == 0) found_origin_at = 0;

			y = (y - found_origin_at ) % y_max_samples;

			profile profile{y,x,z};

			belt_boundary_buffer.push_back(find_belt_edges(profile,nan_num,x_resolution));
			profile_buffer.push_back(profile);

			// Wait for buffers to fill
			if (profile_buffer.size() < fiducial.rows) continue;

	
			if(pending_meta_upload) {
				pending_meta_upload = false;
				auto [z_min,z_max] = find_minmax_z(profile_buffer);

				resolution["z_off"] = -1*z_resolution*z_min;
				std::unique_lock<std::mutex> lock(m);
				upload_fifo.push(resolution.dump());
				lock.unlock();
				sig.notify_one();

				auto hist = histogram(profile_buffer,z_min,z_max);
				cv_threshhold = get<0>(hist[0]) - global_config["fiducial_depth"].get<double>()/z_resolution;
			}

			auto belt = buffers_to_mat(profile_buffer,belt_boundary_buffer,x_resolution);
			bool found_origin = samples_contains_fiducial(belt,fiducial,belt_crosscorr_threshold,cv_threshhold);

			if(found_origin) {
				found_origin_at = y;
				uint64_t i = 0;
				 
				// Reset buffer y values to origin
				for(auto &p : profile_buffer) {
					p.y = i++;
				}
				log->info("Fiducial found at y:{}", y);
			}

			cads::profile front_profile = profile_buffer.front();
			
			const auto previous_profile = fetch_profile(fetch_stmt,front_profile.y);
			
			if(!store_profile(stmt,front_profile)) {
				std::unique_lock<std::mutex> lock(m_db);
				db_fifo.push(front_profile);
				lock.unlock();
				sig_db.notify_one();
			}

			auto is_same = compare_samples(front_profile,previous_profile);
			if(!is_same) {
				std::unique_lock<std::mutex> lock(m);

				if(upload_fifo.size() < y_max_samples) {
					upload_fifo.push(front_profile.y);
					lock.unlock();
					sig.notify_one();
				}
			}

			belt_boundary_buffer.pop_front();
			profile_buffer.pop_front();

		}
		gocator.Stop();
		close_db(db,stmt,fetch_stmt);
	}
}