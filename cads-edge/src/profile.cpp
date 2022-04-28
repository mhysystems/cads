#include <limits>
#include <profile.h>
#include <constants.h>
#include <regression.h>

#include <cmath>

namespace cads {
	
std::tuple<z_element,z_element> find_minmax_z(const profile& ps) {
  
  auto z_min = std::numeric_limits<z_element>::max();
  auto z_max = std::numeric_limits<z_element>::min();

  for(auto z: ps.z) {
    if(z != NaN<z_element>::value) {
      z_min = std::min(z,z_min);
      z_max = std::max(z,z_max);
    }
  }

  return {z_min,z_max};
}

bool compare_samples(const profile& a, const profile& b, int threshold = 100) {
	auto az = a.z;
	auto bz = b.z;
	auto len = std::min(az.size(),bz.size());
	
	int cnt = 0;
	
	// 40 was found to be the variance between z samples of a non moving scan
	for(size_t i = 0; i < len; i++) {
		if(abs(az[i] - bz[i]) > 40) cnt++;
	}

	return cnt < threshold && cnt > len*0.5;
}

} // namespace cads