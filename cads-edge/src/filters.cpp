#include <filters.h>
#include <constants.h>
#include <algorithm>
#include <ranges>
#include <window.hpp>
#include <global_config.h>
#include <ranges>

#include <Iir.h>

namespace cads 
{
  void spike_filter(z_type& z, int window_size) {

    auto z_min = NaN<z_element>::value;

    for(int i = 0; i < z.size()-window_size;) {
      if(z[i] == z_min && z[i+window_size] == z_min) {
        for(int j = i+1; j < i + window_size; ++j) {
          z[j] = z_min;
        }
        i += window_size;
      }else{
        ++i;
      }
    }

    for(int i = z.size()-window_size; i < z.size()-3;) {
      if(z[i] == z_min && z[i+3] == z_min) {
        for(int j = i+1; j < i + 3; ++j) {
          z[j] = z_min;
        }
        i += 3;
      }else{
        ++i;
      }
    }
  }

  void nan_filter(z_type& z) {
    namespace sr = std::ranges;
    
    auto z_min = NaN<z_type::value_type>::value;
    
    auto prev_value_it = sr::find_if(z,[z_min](z_element a){ return a != z_min;}); 
    z_element prev_value = prev_value_it != z.end() ? *prev_value_it : z_min; 
    
    int mid = z.size() / 2;

    for(auto&& e : z | sr::views::take(mid)) {
      if(e != z_min) {
        prev_value = e;
      }else {
        e = prev_value;
      }
    }

    auto prev_value_it2 = sr::find_if(z | sr::views::reverse ,[z_min](z_element a){ return a != z_min;}); 
    prev_value = prev_value_it2 != z.rend() ? *prev_value_it2 : z_min;

    for(auto&& e : z | sr::views::reverse | sr::views::take(mid) ) {
      if(e != z_min) {
        prev_value = e;
      }else {
        e = prev_value;
      }
    }
  }

  void barrel_height_compensate(z_type& z, z_element z_off) {
    for(auto &e : z) {
      e += z_off;
    }
  }

  std::function<double(double)> mk_iirfilter(const std::vector<double> as, const std::vector<double> bs, z_element init) {
    
    std::vector<double>  xs(bs.size()-1,init);
    std::vector<double>  ys(as.size()-1,init);

    return [=](z_element xn) mutable {

      double yn = xs[0]*bs[0] - ys[0]*as[0];
      
      const auto M = xs.size();

      for(int i = 1; i < M; ++i) {
        yn += xs[i]*bs[i] - ys[i]*as[i];
        xs[i-1] = xs[i];
        ys[i-1] = ys[i];

      }

      yn += xn * bs.back();
      yn *= 1 / as.back();
      xs.back() = xn;
      ys.back() = yn;

      return yn;     
    };
  }

  std::function<double(double)> mk_iirfilterSoS() {
    auto coeff = global_config["iirfilter"]["sos"].get<std::vector<std::vector<double>>>();
    auto r = coeff | std::ranges::views::join;
    std::vector<double> in(r.begin(),r.end());
    Iir::Custom::SOSCascade<5> a(*(double (*)[5][6])in.data());

     return [=](z_element xn) mutable {
       return a.filter(xn);
     };
  }

  std::function<std::tuple<bool,std::tuple<y_type,double,z_type>>(std::tuple<y_type,double,z_type>)> mk_delay(size_t len) {
    
    std::deque<std::tuple<y_type,double,z_type>> delay;
    return [=](std::tuple<y_type,double,z_type> p) mutable {
      delay.push_back(p);
      
      if(delay.size() < len) {
        return std::tuple{false,std::tuple<y_type,double,z_type>()};
      }

      auto rn = delay.front();
      delay.pop_front();

      return std::tuple{true,rn};

    };
    
  }



} // namespace cads