#include <algorithm>
#include <ranges>

#if 0
#include <gsl/gsl_math.h>
#include <gsl/gsl_filter.h>
#include <gsl/gsl_rng.h>
#include <gsl/gsl_randist.h>
#include <gsl/gsl_vector.h>
#endif

#include <Iir.h>

#include <filters.h>
#include <constants.h>


namespace cads
{
  void spike_filter(z_type &z, int max_window_size)
  {

    int z_size = (int)z.size();

    if (max_window_size > z_size)
      return;

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

  void spike_filter(z_type &z, int window_size, int lead)
  {

    int z_size = (int)z.size();

    if (window_size > z_size && window_size == 0)
      return;

    for (auto i = 0; i < z_size - 2*lead + 1; i++)
    {

      if (!std::isnan(z[i]))
        continue;

      auto b = 1; // begin
      while(std::isnan(z[i + b]) && (b < lead)) ++b;

      if(b != lead)
        continue;

      for(auto ws = 1; (ws <= window_size) && ((i + 2*lead + ws) < z_size); ws++) {
        if (!std::isnan(z[i + lead + ws + 1]))
          continue;

        auto e = 1; //end
        while(std::isnan(z[i + lead + ws + e]) && (e < lead)) ++e;

        if(e != lead)
          continue;

        for(auto j = 0; j < ws; ++j) {
          z[i + j + lead] = std::numeric_limits<z_element>::quiet_NaN();
        }
      }
    }
  }


  void pulley_level_compensate(z_type &z, z_element z_off, z_element z_max)
  {
    const float threshold = z_max * 0.2;

    for (auto &e : z)
    {
      e += z_off;
      if (e < threshold )
        e = 0;
      if (e > z_max)
        e = z_max;
    }
  }

  std::function<double(double)> mk_iirfilterSoS(std::vector<std::vector<double>> coeff)
  {
    auto r = coeff | std::ranges::views::join;
    std::vector<double> in(r.begin(), r.end());
    Iir::Custom::SOSCascade<10> a(*(double(*)[10][6])in.data());

    return [=](z_element xn) mutable
    {
      return a.filter(xn);
    };
  }

  std::function<std::tuple<bool, std::tuple<profile,int,int,int,z_type>>(std::tuple<profile,int,int,int,z_type>)> mk_delay(size_t len)
  {

    std::deque<std::tuple<profile,int,int,int,z_type>> delay;
    return [=](std::tuple<profile,int,int,int,z_type> p) mutable
    {
      delay.push_back(p);

      if (delay.size() < len)
      {
        return std::tuple{false, std::tuple<profile,int,int,int,z_type>()};
      }

      auto rn = delay.front();
      delay.pop_front();

      return std::tuple{true, rn};
    };
  }

  std::function<cads::z_element(cads::z_element)> mk_schmitt_trigger(const cads::z_element ref, const cads::z_element bias)
  {

    auto level = true;

    return [=](cads::z_element x) mutable
    {
      auto high = x > (bias+ref);
      auto low =  x < (bias -ref);
      level = high || (level && !low);
      return level ? cads::z_element(1) : cads::z_element(-1);
    };
  }

  std::function<cads::z_element(cads::z_element)> mk_schmitt_trigger(const cads::z_element bias)
  {
    return mk_schmitt_trigger(global_filters.SchmittThreshold,bias);
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
  
#if 0
  void gaussian(z_type& z) 
  {
    gsl_vector *x = ::gsl_vector_alloc(z.size());
    gsl_vector *yv = ::gsl_vector_alloc(z.size());
    gsl_filter_gaussian_workspace *gauss_p = ::gsl_filter_gaussian_alloc(51);

    for(auto i=0; i < z.size(); i++) {
      gsl_vector_set(x, i,z[i]);
    }

    ::gsl_filter_gaussian(GSL_FILTER_END_PADVALUE, 10.0, 0, x, yv, gauss_p);

    for(auto i=0; i < z.size(); i++) {
      //if(!std::isnan(z[i])) {
        z[i] = gsl_vector_get(yv, i);
      //}
    }

    gsl_vector_free(x);
    gsl_vector_free(yv);
    gsl_filter_gaussian_free(gauss_p);
  }
#endif
} // namespace cads







