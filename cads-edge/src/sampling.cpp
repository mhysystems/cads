#include <ranges>

#include <spline.h>

#include <sampling.h>
#include <utils.hpp>

namespace cads
{
  z_type decimate(z_type z, double stride) 
  {
    auto size = z.size();
    size_t c = 0;
    for (double i = 0; std::size_t(i) < size; i+= stride)
    {
      z[c++] = z[std::size_t(i)];
    }
    z.erase(z.begin()+c,z.end());
    return z;
  }

  decltype(cads::profile::z) interpolate_to_widthn(decltype(cads::profile::z) z, size_t n )
  {
    using namespace std;

    auto step = (double)z.size() / (double)n;
    decltype(z) interpolated_z(n);

    assert(step > 0);

    for(size_t i = 0; i < n; i++) {
      interpolated_z[i] = z[(size_t)floor(i*step)];
    }

    return interpolated_z;
  }

  decltype(cads::profile::z) interpolation_nearest(decltype(cads::profile::z) z)
  {
    using namespace std;
    namespace sr = std::ranges;
    
    auto z_range = z | sr::views::filter([](float e) { return !std::isnan(e); });
    decltype(cads::profile::z) filtered_z{z_range.begin(),z_range.end()};
    auto step = (double)z.size() / (double)filtered_z.size();
    
    assert(step > 0);
    
    for(size_t i = 0; i < z.size(); i++) {
      z[i] = filtered_z[(size_t)floor(i*step)];  
    }

    return z;
  }

  void interpolation_nearest(z_type::iterator begin, z_type::iterator end, std::function<bool(z_element)> is)
  {

    if (begin >= end) return;

    for (auto i = begin; i < end;)
    {
      i = std::find_if(i, end, is);
      auto iv = i == begin ? *i : *(i - 1); 

      auto n = std::find_if_not(i,end,is);
      auto nv = n == end ? *(n - 1) : *n;

      auto inc = !is(iv);
      auto dec = !is(nv);

      if(!inc && !dec) break;

      while(i <= n) {
        *i = iv;
        *n = nv;

        i += 1*inc;
        n -= 1*dec;
      }
    }
  }

  zrange nan_interpolation_spline(zrange z)
  {
    std::vector<double> x, y;

    double cnt = 0;
    for (auto i : z)
    {
      if (!std::isnan(i))
      {
        x.push_back(cnt);
        y.push_back(i);
      }

      ++cnt;
    }

    tk::spline s(x, y);

    for (double i = 0; i < (double)z.size(); ++i)
    {
      *(z.begin() + (int)i) = s(i);
    }

    return z;
  }

  void nan_interpolation_spline(z_type &z)
  {

    std::vector<double> x, y;

    double cnt = 0;
    for (auto i : z)
    {
      if (!std::isnan(i))
      {
        x.push_back(cnt);
        y.push_back(i);
      }

      ++cnt;
    }

    tk::spline s(x, y);

    for (double i = 0; i < (double)z.size(); ++i)
    {
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

    auto prev_value_it = std::find_if(begin, end, [](z_element a)
                                      { return !std::isnan(a); });
    z_element prev_value = prev_value_it != end ? *prev_value_it : std::numeric_limits<z_element>::quiet_NaN();

    auto e = begin;
    while (e++ < end)
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

  void nan_interpolation_mean(z_type &z)
  {

    float p = 0;
    for (auto i = z.begin(); i < z.end();)
    {
      i = std::find_if(i, z.end(), [](z_element a)
                       { return std::isnan(a); });
    

      auto cnt = 10;
      auto l = i;
      while (l >= z.begin() && cnt > 0)
      {
        if (!std::isnan(*l))
        {
          --cnt;
        }
        --l;
      }

      cnt = 10;

      auto r = i;
      while (r < z.end() && cnt > 0)
      {
        if (!std::isnan(*r))
        {
          --cnt;
        }
        ++r;
      }

      auto v = interquartile_mean(std::ranges::subrange(l, r));
      if (!std::isnan(v))
      {
        *i = v;
        p = v;
      }
      else
      {
        *i = p;
      }
    }
  }
}