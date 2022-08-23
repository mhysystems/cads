#include <filters.h>
#include <constants.h>
#include <algorithm>
#include <ranges>
#include <window.hpp>

#include <Iir.h>

namespace cads 
{
  void spike_filter2(z_element* z, int window_size) {
    
    for(auto i = 0; i < window_size;i++) {
      if(NaN<z_element>::isnan(z[i]) && NaN<z_element>::isnan(z[i+window_size])) {
        for(auto j = i+1; j < i + window_size; ++j) {
          z[j] = NaN<z_element>::value;
        }
      }
    }
  }

  void spike_filter(z_type& z, int window_size) {
    
    int z_size = (int)z.size();

    if(window_size > z_size) return;

    for(auto i = 0; i < z_size-window_size;i+=window_size) {
       for(auto j = 3; j < window_size; j++) {
        spike_filter2(&z[i],j);
      }
    }
  }


  void nan_filter(z_type& z) {
    namespace sr = std::ranges;
    
    auto prev_value_it = sr::find_if(z,[](z_element a){ return !NaN<z_element>::isnan(a);}); 
    z_element prev_value = prev_value_it != z.end() ? *prev_value_it : NaN<z_element>::value; 
    
    int mid = (int)(z.size() / 2);

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
    const auto z_max_compendated = z_max + z_off;

    for(auto &e : z) {
      e += z_off;
      if(e < 0) e = 0;
      if(e > z_max_compendated) e = z_max_compendated;
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

  std::function<cads::z_element(cads::z_element)> mk_schmitt_trigger(const cads::z_element ref) {

    auto level = true;    
    
    return [=](cads::z_element x) mutable {
      auto high = x > ref;
      auto low  = x < -ref;
      level = high || (level && !low);
      return level ? cads::z_element(1) : cads::z_element(-1);
    };

  }

} // namespace cads