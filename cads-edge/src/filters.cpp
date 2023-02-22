#include <filters.h>
#include <constants.h>
#include <algorithm>
#include <ranges>
#include <window.hpp>

#include <Iir.h>
#include <spline.h>

namespace cads
{
  void spike_filter(z_type &z, int max_window_size)
  {

    int z_size = (int)z.size();

    if (max_window_size > z_size)
      return;

    // Force start with NaN. Can get poor quality single value at the start of profile before chain of NaN.
    z[0] = std::numeric_limits<z_element>::quiet_NaN();

    for (auto i = 0; i < z_size - 3; i++)
    {

      if (!std::isnan(z[i]))
        continue;

      for (auto j = 3; j <= std::min(z_size - i, max_window_size); j++)
      {
        if (std::isnan(z[i + j - 1]))
        {
          for (auto k = i + 1; k < i + j; ++k)
          {
            z[k] = std::numeric_limits<z_element>::quiet_NaN();
          }
          i += j - 1 - 1; // Extra -1 is due to for(...,i++)
          break;
        }
      }
    }
  }


  z_type nan_filter_pure(z_type z) {
    nan_filter(z);
    return z;
  }

  void nan_filter_spline(z_type &z) {
    
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

  void nan_filter(z_type &z)
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

  void barrel_height_compensate(z_type &z, z_element z_off, z_element z_max)
  {
    const auto z_max_compendated = z_max + z_off;

    for (auto &e : z)
    {
      e += z_off;
      if (e < 5.0f )
        e = 0;
      if (e > z_max_compendated)
        e = z_max_compendated;
    }
  }

  void constraint_clipping(z_type& z, z_element z_min, z_element z_max) {
    for (auto &e : z)
    {
      if(!std::isnan(e)) {
        if (e < z_min )
          e = z_min;
        if (e > z_max)
          e = z_max;
      }
    } 
  }

  std::function<double(double)> mk_iirfilterSoS()
  {
    auto coeff = global_config["iirfilter"]["sos"].get<std::vector<std::vector<double>>>();
    auto r = coeff | std::ranges::views::join;
    std::vector<double> in(r.begin(), r.end());
    Iir::Custom::SOSCascade<10> a(*(double(*)[10][6])in.data());

    return [=](z_element xn) mutable
    {
      return a.filter(xn);
    };
  }

  std::function<std::tuple<bool, std::tuple<y_type, double, z_type,int,int,z_type>>(std::tuple<y_type, double, z_type,int,int,z_type>)> mk_delay(size_t len)
  {

    std::deque<std::tuple<y_type, double, z_type,int,int,z_type>> delay;
    return [=](std::tuple<y_type, double, z_type,int,int,z_type> p) mutable
    {
      delay.push_back(p);

      if (delay.size() < len)
      {
        return std::tuple{false, std::tuple<y_type, double, z_type,int,int,z_type>()};
      }

      auto rn = delay.front();
      delay.pop_front();

      return std::tuple{true, rn};
    };
  }

  std::function<cads::z_element(cads::z_element)> mk_schmitt_trigger(const cads::z_element ref)
  {

    auto level = true;

    return [=](cads::z_element x) mutable
    {
      auto high = x > ref;
      auto low = x < -ref;
      level = high || (level && !low);
      return level ? cads::z_element(1) : cads::z_element(-1);
    };
  }

  std::function<cads::z_element(cads::z_element)> mk_schmitt_trigger()
  {
    return mk_schmitt_trigger(global_filters.SchmittThreshold);
  }

  std::function<cads::z_element(cads::z_element,bool)> mk_amplitude_extraction()
  {
    cads::z_element min = std::numeric_limits<cads::z_element>::max();
    cads::z_element max = std::numeric_limits<cads::z_element>::lowest();

    return [=](cads::z_element x, bool reset) mutable
    {
      auto r = max - min;
      if (!reset)
      {
        min = std::min(min, x);
        max = std::max(max, x);
        r = max - min;
      }
      else
      {
        min = std::numeric_limits<cads::z_element>::max();
        max = std::numeric_limits<cads::z_element>::lowest();
      }

      return r;
    };
  }

  std::function<double(double)> mk_dc_filter(){
    cads::z_element yn = 0, xn = 0;
    // https://www.embedded.com/dsp-tricks-dc-removal/
    return [=](double x) mutable
    {
        auto y = x - xn + 0.995f * yn;
        xn = x;
        yn = y;
        return y;
    };
  }

} // namespace cads