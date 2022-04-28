#include <window.hpp>

#include <cmath>
#include <ranges>
#include <constants.h>

#include <opencv2/core.hpp>

using namespace std;

namespace cads {


tuple<double,double> x_minmax(window win, double x_res) {
  double x_min = numeric_limits<double>::max();
  double x_max = numeric_limits<double>::min();
  
  for(auto p : win) {
    x_min = min(x_min,p.x_off);
    x_max = max(x_max,p.x_off + p.z.size()*x_res);
  }

  return {x_min,x_max};
}


cv::Mat window_to_mat(const window& win, double x_res) {

  if(win.size() < 1) return cv::Mat(0,0,CV_32F);

	auto [x_min,x_max] = x_minmax(win,x_res);

	const int n_cols = (int)ceil((x_max-x_min) / x_res);
  const auto n_rows = win.size();
	
  cv::Mat mat(n_rows,n_cols,CV_32F,cv::Scalar::all(0.0f));
	
  int i = 0;
  for(auto p : win) {

    auto m = mat.ptr<float>(i++);

    int j = (p.x_off - x_min) / x_res;
    
    for(auto z : p.z) {
       m[j++] = (float)z;
    }
  }

	return mat;
}

std::tuple<z_element,z_element> find_minmax_z(const window& ps) {
  auto z_min = std::numeric_limits<z_element>::max();
  auto z_max = std::numeric_limits<z_element>::min();
  
  for(auto p : ps) {
    for(auto z: p.z) {
      if(z != NaN<z_element>::value) {
        z_min = std::min(z,z_min);
        z_max = std::max(z,z_max);
      }
    }
  }

  return {z_min,z_max};
}

vector<tuple<double,z_element>> histogram(const window& ps, z_element min, z_element max, double size) {

  const auto dz = (size-1) / (max - min);
  vector<tuple<double,z_element>> hist(size,{0,0}); 

  for(auto p : ps) {
    auto zs = p.z;
    for(auto z: zs) {
      if(z != NaN<z_element>::value) {
        int i = (z - min)*dz;
        hist[i] = {(i + 2)*(1/dz)+min,1+get<1>(hist[i])};
      }
    }
  }

  ranges::sort(hist,[](auto a, auto b){ return get<1>(a) > get<1>(b);});
  return hist;
}

double left_edge_avg_height(const cv::Mat& belt, const cv::Mat& fiducial) {
  double minVal, maxVal;
  cv::minMaxLoc( belt.colRange(0,fiducial.cols*1.5), &minVal, &maxVal);
  cv::Mat mout;
  cv::multiply(belt.colRange(0,fiducial.cols),fiducial,mout);
  auto avg_val = cv::sum(mout)[0] / cv::countNonZero(mout);
  return avg_val;
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
#endif


} // namespace cads