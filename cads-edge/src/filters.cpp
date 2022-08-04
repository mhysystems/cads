#include <filters.h>
#include <constants.h>
#include <algorithm>
#include <ranges>
#include <window.hpp>

#include <Iir.h>

namespace cads 
{
  void spike_filter(z_type& z, int window_size) {
    
    int z_size = (int)z.size();

    if(window_size > z_size) return;

    for(auto i = 0; i < z_size-window_size;) {
      if(NaN<z_element>::isnan(z[i]) && NaN<z_element>::isnan(z[i+window_size])) {
        for(auto j = i+1; j < i + window_size; ++j) {
          z[j] = NaN<z_element>::value;
        }
        i += window_size;
      }else{
        ++i;
      }
    }

    for(auto i = z_size-window_size; i < z_size-3;) {
      if(NaN<z_element>::isnan(z[i]) && NaN<z_element>::isnan(z[i+3])) {
        for(auto j = i+1; j < i + 3; ++j) {
          z[j] = NaN<z_element>::value;
        }
        i += 3;
      }else{
        ++i;
      }
    }
  }

  void nan_filter(z_type& z) {
    namespace sr = std::ranges;
    
    auto prev_value_it = sr::find_if(z,[](z_element a){ return !NaN<z_element>::isnan(a);}); 
    z_element prev_value = prev_value_it != z.end() ? *prev_value_it : NaN<z_element>::value; 
    
    int mid = z.size() / 2;

    for(auto&& e : z | sr::views::take(mid)) {
      if(!NaN<z_element>::isnan(e)) {
        prev_value = e;
      }else {
        e = prev_value;
      }
    }

    auto prev_value_it2 = sr::find_if(z | sr::views::reverse ,[](z_element a){ return !NaN<z_element>::isnan(a);}); 
    prev_value = prev_value_it2 != z.rend() ? *prev_value_it2 : NaN<z_element>::value;

    for(auto&& e : z | sr::views::reverse | sr::views::take(mid+1)) {
      if(!NaN<z_element>::isnan(e)) {
        prev_value = e;
      }else {
        e = prev_value;
      }
    }
  }

  void barrel_height_compensate(z_type& z, z_element z_off, z_element z_max) {
    for(auto &e : z) {
      e += z_off;
      if(e < 0) e = 0;
      if(e > z_max) e = z_max;
    }
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