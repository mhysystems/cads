#include <limits>
#include <cmath>
#include <vector>
#include <ranges>
#include <tuple>
#include <algorithm>
#include <numeric>

#include <profile.h>
#include <constants.h>
#include <regression.h>



using namespace std;

namespace cads {
	
std::tuple<z_element,z_element> find_minmax_z(const z_type& ps) {
  
  auto z_min = std::numeric_limits<z_element>::max();
  auto z_max = std::numeric_limits<z_element>::lowest();

  for(auto z: ps) {
    if(!NaN<z_element>::isnan(z)) {
      z_min = std::min(z,z_min);
      z_max = std::max(z,z_max);
    }
  }

  return {z_min,z_max};
}

bool compare_samples(const profile& a, const profile& b, int threshold = 100) {
	auto az = a.z;
	auto bz = b.z;
	auto len = (int)std::min(az.size(),bz.size());
	
	long cnt = 0;
	
	// 40 was found to be the variance between z samples of a non moving scan
	for(int i = 0; i < len; i++) {
		if(abs(az[i] - bz[i]) > 40) cnt++;
	}

	return cnt < threshold && cnt > int(len*0.5);
}

vector<tuple<double,z_element>> histogram(const z_type& ps, z_element min, z_element max, double size) {

  const float dz = ((float)size-1) / (max - min);
  vector<tuple<double,z_element>> hist((size_t)size,{0,0.0f}); 


  for(auto z: ps) {
    if(!NaN<z_element>::isnan(z)) {
      int i = int((z - min)*dz);
      hist[i] = {((float)i + 2)*(1/dz)+min,1+get<1>(hist[i])};
    }
  }

  ranges::sort(hist,[](auto a, auto b){ return get<1>(a) > get<1>(b);});
  return hist;
}

tuple<z_element,z_element,bool> barrel_offset(const z_type& win,double z_height_mm) {

    
  auto [z_min,z_max] = find_minmax_z(win);
  
  // Histogram, first is z value, second is count 
  auto hist = histogram(win,z_min,z_max,100);
  
  const auto peak = get<0>(hist[0]);
  
  // Remove z values greater than the peak minus approx belt thickness.
  // Assumes the next peak will be the barrel values
  auto f = hist | views::filter([z_height_mm,peak](tuple<double,z_element> a ){ return peak - get<0>(a) > z_height_mm; });

  if(f.begin() != f.end()) {
    return {get<0>(*f.begin()),peak,false};
  }else {
    return {peak - z_height_mm,peak,true};
  }
}

  bool operator==(const profile& a, const profile& b) {
    auto tx = a.x_off == b.x_off;
    auto ty = a.y == b.y;
    auto tz = std::ranges::equal(a.z,b.z);
    return tx && ty && tz;
  }

} // namespace cads