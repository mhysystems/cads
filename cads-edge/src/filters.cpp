#include <limits>
#include <deque>
#include <tuple>
#include <ranges>

#include <Iir.h>

#include <filters.h>



namespace cads
{
  void pulley_level_compensate(z_type &z, z_element z_off, z_element z_max, z_element threshold)
  {

    for (auto &e : z)
    {
      e += z_off;
      if (e < threshold || std::isnan(e))
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

  std::function<std::tuple<bool, std::tuple<profile,int,int,int,int>>(std::tuple<profile,int,int,int,int>)> mk_delay(size_t len)
  {

    std::deque<std::tuple<profile,int,int,int,int>> delay;
    return [=](std::tuple<profile,int,int,int,int> p) mutable
    {
      delay.push_back(p);

      if (delay.size() < len)
      {
        return std::tuple{false, std::tuple<profile,int,int,int,int>()};
      }

      auto rn = delay.front();
      delay.pop_front();

      return std::tuple{true, rn};
    };
  }

  std::function<cads::z_element(cads::z_element,cads::z_element)> mk_schmitt_trigger(const cads::z_element ref)
  {

    auto level = true;

    return [=](cads::z_element x, cads::z_element bias) mutable
    {
      auto high = x > (bias+ref);
      auto low =  x < (bias -ref);
      level = high || (level && !low);
      return level ? cads::z_element(1) : cads::z_element(-1);
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







