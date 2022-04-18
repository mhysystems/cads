#include <generator.hpp>
#include <chrono>


#include <z_data_generated.h>
#include <p_config_generated.h>

using namespace std;
using namespace moodycamel;
using namespace std::chrono;

namespace cads
{


  gocator_profile extractZData(std::unique_ptr<char[]> buf) {

  using namespace flatbuffers;

  auto profile = cads_flatworld::Getprofile(buf.get());
  auto y = profile->frame();
  auto x_off = profile->x_off();
  auto z_samples = profile->z_samples();
  std::vector<int16_t> z(z_samples->begin(),z_samples->end());

  return {y,x_off,z};

  }

  generator<gocator_profile> get_flatworld(BlockingReaderWriterQueue<char>& fifo) {

	auto init = std::chrono::high_resolution_clock::now() ;
  auto milliseconds = std::chrono::duration_cast<std::chrono::milliseconds>(init - init).count();
  auto pt = milliseconds;
  auto start = init;
  auto end = start;
  uint64_t cnt = 0;
  
  while(true) {
		int size = 0;

    if(cnt++ > 2000) {
    //  glog->info("Average Wait in milliseconds:{}, Avg processing time:{}",milliseconds,pt);
    //  spdlog::info("Average Wait in milliseconds:{}, Avg processing time:{}",milliseconds,pt);
      cnt = 0;
    } 
		
		start = std::chrono::high_resolution_clock::now();
    pt = (pt + std::chrono::duration_cast<std::chrono::milliseconds>(start - end).count()) / 2;
    fifo.wait_dequeue((char*)&size,sizeof(size));
    end = std::chrono::high_resolution_clock::now();
    milliseconds = (milliseconds + std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count()) / 2;


		if(size < 1) {
			break;
		} 

    if(cnt == 0) {
     // spdlog::info("frame bytes:{}",size);
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

std::tuple<double,double,double,double> get_gocator_constants(BlockingReaderWriterQueue<char>& fifo) {
	
	int size = 0;
	
	fifo.wait_dequeue((char*)&size,sizeof(size));

	auto buf = std::make_unique<char[]>(size);
	
	fifo.wait_dequeue(buf.get(),size);

	return getProfile(std::move(buf));

}



}