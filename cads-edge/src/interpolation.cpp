#include <ranges>

#include <spline.h>

#include <interpolation.h>
#include <utils.hpp>

namespace cads
{
  
  zrange nan_interpolation_spline(zrange z) {
    std::vector<double> x,y;

    double cnt = 0;
    for(auto i : z) {
      if(!std::isnan(i)) {
        x.push_back(cnt);
        y.push_back(i);
      }

      ++cnt;
    }

    tk::spline s(x,y);

    for(double i = 0; i < (double)z.size(); ++i) {
      *(z.begin() + (int)i) = s(i);
    }

    return z;
    
  }
  
  void nan_interpolation_spline(z_type &z) {
    
    std::vector<double> x,y;
    
    double cnt = 0;
    for(auto i : z) {
      if(!std::isnan(i)) {
        x.push_back(cnt);
        y.push_back(i);
      }

      ++cnt;
    }

    tk::spline s(x,y);

    for(double i = 0; i < (double)z.size(); ++i) {
      z[(int)i] = s(i);
    }
    
  }

  void nan_interpolation_last(z_type &z)
  {
    namespace sr = std::ranges;

    auto prev_value_it = sr::find_if(z, [](z_element a)
                                     { return !std::isnan(a); });
    z_element prev_value = prev_value_it != z.end() ? *prev_value_it : std::numeric_limits<z_element>::quiet_NaN();

    int mid = (int)(z.size() / 2);

    for (auto &&e : z | sr::views::take(mid))
    {
      if (!std::isnan(e))
      {
        prev_value = e;
      }
      else
      {
        e = prev_value;
      }
    }

    auto prev_value_it2 = sr::find_if(z | sr::views::reverse, [](z_element a)
                                      { return !std::isnan(a); });
    prev_value = prev_value_it2 != z.rend() ? *prev_value_it2 : std::numeric_limits<z_element>::quiet_NaN();

    for (auto &&e : z | sr::views::reverse | sr::views::take(mid + 1))
    {
      if (!std::isnan(e))
      {
        prev_value = e;
      }
      else
      {
        e = prev_value;
      }
    }
  }

  void nan_interpolation_last(z_type::iterator begin, z_type::iterator end)
  {

    auto prev_value_it = std::find_if(begin,end, [](z_element a)
                                     { return !std::isnan(a); });
    z_element prev_value = prev_value_it != end ? *prev_value_it : std::numeric_limits<z_element>::quiet_NaN();

    auto e = begin;
    while(e++ < end)
    {
      if (!std::isnan(*e))
      {
        prev_value = *e;
      }
      else
      {
        *e = prev_value;
      }
    }
  }

  void nan_interpolation_mean(z_type &z) {

    float p = 0;
    for(auto i = z.begin(); i < z.end();) {
      i = std::find_if(i,z.end(), [](z_element a) { return std::isnan(a); });
      auto l = i;
      
      auto cnt = 10;
      
      while(l >= z.begin() && cnt > 0) {
        if(!std::isnan(*l)) {
          --cnt;
        }
        --l;
      }

      cnt = 10;

      auto r = i;
      while(r < z.end() && cnt > 0) {
        if(!std::isnan(*r)) {
          --cnt;
        }
        ++r;
      }

      auto v = interquartile_mean(std::ranges::subrange(l,r));
      if(!std::isnan(v)) {
        *i = v;
        p = v;
      }else{
        *i = p;
      }

    }

  }


}