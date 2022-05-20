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

  while(true) {
		int size = 0;

    fifo.wait_dequeue((char*)&size,sizeof(size));

		if(size < 1) {
			break;
		} 

		auto buf = std::make_unique<char[]>(size);
		fifo.wait_dequeue(buf.get(),size);
    auto p = extractZData(std::move(buf));
		auto y = std::get<0>(p);
    
    if(y == std::numeric_limits<decltype(y)>::max()) {
      break;
    }
    
    co_yield p;
	}
}

  generator<gocator_profile> get_flatworld(BlockingReaderWriterQueue<profile>& fifo) {

  while(true) {
		profile p;

    fifo.wait_dequeue(p);
    if(p.y == std::numeric_limits<decltype(p.y)>::max()) {
      break;
    }

		co_yield {p.y,p.x_off,p.z};
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