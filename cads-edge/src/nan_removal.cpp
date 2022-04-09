#include <vector>
#include <cstdint>
#include <ranges>
#include <algorithm>


namespace cads {

  std::vector<int16_t> nan_removal(std::vector<int16_t> x, int16_t z_min) {
    namespace sr = std::ranges;
    
    auto prev_value_it = sr::find_if(x,[z_min](int16_t a){ return a >= z_min;}); 
    int16_t prev_value = prev_value_it != x.end() ? *prev_value_it : z_min; 
    
    
    int mid = x.size() / 2;

    for(auto&& e : x | sr::views::take(mid)) {
      if(e >= z_min) {
        prev_value = e;
      }else {
        e = prev_value;
      }
    }

    auto prev_value_it2 = sr::find_if(x | sr::views::reverse ,[z_min](int16_t a){ return a >= z_min;}); 
    prev_value = prev_value_it2 != x.rend() ? *prev_value_it2 : z_min;

    for(auto&& e : x | sr::views::reverse | sr::views::take(mid) ) {
      if(e >= z_min) {
        prev_value = e;
      }else {
        e = prev_value;
      }
    }

    return x;
  }

}