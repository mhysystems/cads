#include <window.hpp>

#include <tuple>
#include <cmath>

using namespace std;

namespace cads {


tuple<double,double>  x_minmax(window win, double x_res) {
  double x_min = numeric_limits<double>::max();
  double x_max = numeric_limits<double>::min();
  
  for(auto p : win) {
    x_min = min(x_min,p.x_off);
    x_max = max(x_max,p.x_off + p.z.size()*x_res);
  }

  return {x_min,x_max};
}


cv::Mat window_to_mat(window win, double x_res) {

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




}