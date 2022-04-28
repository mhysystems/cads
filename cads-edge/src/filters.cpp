#include <filters.h>
#include <constants.h>
#include <algorithm>
#include <ranges>

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

} // namespace cads