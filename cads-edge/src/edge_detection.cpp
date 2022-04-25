#include <edge_detection.h>
#include <algorithm>
#include <limits>
#include <constants.h>
#include <ranges>


namespace cads 
{

template<typename T> int edge(T s, T e,int len);

template<typename T> int belt(T s, T e,int len) {

  namespace sr = std::ranges;
  auto z_min = NaN<z_type::value_type>::value;

  auto r = sr::find_if(s,e,[z_min](z_element z) {return z == z_min;} );
  auto d = sr::distance(s,r);
  if(r == e) return d;
  else return d + edge(r,e,len);

}

template<typename T> int edge(T s, T e,int len) {

  namespace sr = std::ranges;
  auto z_min = NaN<z_type::value_type>::value;

  auto r = sr::find_if(s,e,[z_min](z_element z) {return z != z_min;} );
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

std::tuple<int,int> find_profile_edges_nans(const z_type& z, int len) {

  auto mid = int(z.size() / 2);
  auto r = mid + belt(z.begin()+mid,z.end(),len) - 1;
  auto l = z.size() - mid - belt(z.rbegin() + mid ,z.rend(),len);
  return std::tuple<int,int>{l, r};
}


std::tuple<int,int> find_profile_edges_sobel(const z_type& z, int len, int x_width) {

  std::vector<double> win(len*2 + 1,0.0);
  std::fill(win.begin(),win.begin()+len,-1.0);
  std::fill(win.rbegin(),win.rbegin()+len,1.0);
	
  auto min = std::numeric_limits<double>::max();
	auto max = std::numeric_limits<double>::min();
  int left = 0; // Left edge of belt
  int right = 0; // Right edge of belt

  for(int i = 0; i < z.size() - win.size(); i++) {

    double sum = 0.0;
    for(int j = 0; j < win.size(); j++) {
      sum += win[j] * z[i+j];
    }

    if(sum > max) {
      max = sum;
      left = i+len; 
    }
    
    if(sum < min) {
      min = sum;
      right = i+len; 
    }

    if(right - left < 0.9*x_width) {
      right = z.size()-1;
    }

  }

  return std::tuple<int,int>{left, right};

}


} // namespace cads