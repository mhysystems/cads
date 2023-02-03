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

  auto r = sr::find_if(s,e,[](z_element z) {return std::isnan(z);} );
  auto d = sr::distance(s,r);
  if(r == e) return d;
  else return d + edge(r,e,len);

}

template<typename T> int edge(T s, T e,int len) {

  namespace sr = std::ranges;

  auto r = sr::find_if(s,e,[](z_element z) {return !std::isnan(z);} );
  auto d = sr::distance(s,r);

  if(r == e) {
    return 0;
  }
  else if(d >= len) {
    return d;
  }
  else{
    auto window_len = s + len > e ? sr::distance(s,e) : len;
    auto nan_count = (double)sr::count_if(s,s + window_len, [](z_element e) {return std::isnan(e);});
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


std::tuple<int,int> find_profile_edges_nans_outer(const z_type& z, int len) {
  
  auto l = belt(z.begin(),z.end(),len) ;
  auto cl = std::find_if(z.begin()+l,z.end(),[](z_element z) {return !std::isnan(z);});
  l = std::distance(z.begin(),cl);

  if(l >= (int)z.size() / 2) {
    l = 0;
  }

  auto r = belt(z.rbegin(),z.rend(),len);
  auto rl = std::find_if(z.rbegin()+r,z.rend(),[](z_element z) {return !std::isnan(z);});
  r = z.size() - std::distance(z.rbegin(),rl);
  
  if(r >= (int)z.size()) {
    r = z.size();
  }

  return std::tuple<int,int>{l, r};
}


std::tuple<int,int> find_profile_edges_sobel(const z_type& z, int len) {

  std::vector<double> win(len*2 + 1,0.0);
  std::fill(win.begin(),win.begin()+len,-1.0);
  std::fill(win.rbegin(),win.rbegin()+len,1.0);
	
  auto min = std::numeric_limits<double>::max();
	auto max = std::numeric_limits<double>::lowest();
  int left = 0; // Left edge of belt
  int right = 0; // Right edge of belt

  for(int i = 0; i < (int)(z.size() - win.size()); i++) {

    double sum = 0.0;
    for(int j = 0; j < (int)win.size(); j++) {
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
  }
  
  return std::tuple<int,int>{left, right};
}

} // namespace cads