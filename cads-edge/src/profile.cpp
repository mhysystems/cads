#include <limits>
#include <cmath>
#include <vector>
#include <ranges>
#include <tuple>
#include <algorithm>
#include <numeric>

#include <profile.h>
#include <constants.h>
#include <regression.h>
#include <edge_detection.h>
#include <stats.hpp>

namespace cads
{

  
  std::tuple<z_type,z_type> partition_profile(const z_type& z, int left_edge_index, int right_edge_index) {
    using namespace std::ranges;

    auto left_edge = z | views::take(left_edge_index);
    auto right_edge = z | views::drop(right_edge_index);
    auto belt = z | views::take(right_edge_index) | views::drop(left_edge_index);
    
    z_type barrel(left_edge.begin(),left_edge.end());

    barrel.insert(barrel.end(),right_edge.begin(),right_edge.end());

    return {barrel,z_type(belt.begin(),belt.end())};

  }

  auto pulley_mean(z_type &z) 
  {
    using namespace std::ranges;

    auto [q1,q3] = interquartile_range(z);
    auto quartile_filtered = z | views::filter([=](z_element a){ return a >= q1 && a <= q3;});
    auto sum = std::reduce(quartile_filtered.begin(),quartile_filtered.end());
    auto count = (double)std::ranges::distance(quartile_filtered.begin(),quartile_filtered.end());
    auto mean = sum / count;

    return mean;

  }

  std::tuple<double,double> pulley_left_right_mean(const z_type& z, int left_edge_index, int right_edge_index)
  {
    using namespace std::ranges;

    auto left_edge = z | views::take(left_edge_index) | views::filter([](z_element a) { return !std::isnan(a); });
    auto right_edge = z | views::drop(right_edge_index) | views::filter([](z_element a) { return !std::isnan(a); });
    
    auto tl = z_type(left_edge.begin(),left_edge.end());
    auto tr = z_type(right_edge.begin(),right_edge.end());
    
    return {pulley_mean(tl),pulley_mean(tr)};

  }

  double barrel_mean(const z_type& z, int left_edge_index, int right_edge_index) {
      
      namespace sr = std::ranges;

      auto[barrel,belt] = partition_profile(z, left_edge_index, right_edge_index);

      auto [q1,q3] = interquartile_range(barrel);
      auto barrel_no_nans = barrel | sr::views::filter([=](z_element a){ return a >= q1 && a <= q3;});
      
      auto sum = std::reduce(barrel_no_nans.begin(),barrel_no_nans.end());
      auto count = (double)std::ranges::distance(barrel_no_nans.begin(),barrel_no_nans.end());
      
      auto mean = sum / count;

      return mean;

  }
  
  std::tuple<z_element, z_element> find_minmax_z(const z_type &ps)
  {

    auto z_min = std::numeric_limits<z_element>::max();
    auto z_max = std::numeric_limits<z_element>::lowest();

    for (auto z : ps)
    {
      if (!std::isnan(z))
      {
        z_min = std::min(z, z_min);
        z_max = std::max(z, z_max);
      }
    }

    return {z_min, z_max};
  }

  bool compare_samples(const profile &a, const profile &b, int threshold = 100)
  {
    auto az = a.z;
    auto bz = b.z;
    auto len = (int)std::min(az.size(), bz.size());

    long cnt = 0;

    // 40 was found to be the variance between z samples of a non moving scan
    for (int i = 0; i < len; i++)
    {
      if (abs(az[i] - bz[i]) > 40)
        cnt++;
    }

    return cnt < threshold && cnt > int(len * 0.5);
  }

  std::vector<std::tuple<double, z_element>> histogram(const z_type &ps, z_element min, z_element max, double size)
  {

    const float dz = ((float)size - 1) / (max - min);
    std::vector<std::tuple<double, z_element>> hist((size_t)size, {0, 0.0f});

    for (auto z : ps)
    {
      if (!std::isnan(z))
      {
        int i = int((z - min) * dz);
        hist[i] = {((float)i + 2) * (1 / dz) + min, 1 + get<1>(hist[i])};
      }
    }

    std::ranges::sort(hist, [](auto a, auto b)
                 { return get<1>(a) > get<1>(b); });
    return hist;
  }

  std::tuple<z_element, z_element, bool> barrel_offset(const z_type &win, double z_height_mm)
  {

    auto [z_min, z_max] = find_minmax_z(win);

    // Histogram, first is z value, second is count
    auto hist = histogram(win, z_min, z_max, 100);

    const auto peak = std::get<0>(hist[0]);

    // Remove z values greater than the peak minus approx belt thickness.
    // Assumes the next peak will be the barrel values
    auto f = hist | std::ranges::views::filter([z_height_mm, peak](std::tuple<double, z_element> a)
                                  { return peak - get<0>(a) > z_height_mm; });

    if (f.begin() != f.end())
    {
      return {std::get<0>(*f.begin()), peak, false};
    }
    else
    {
      return {peak - z_height_mm, peak, true};
    }
  }


  double barrel_gradient(const z_type &z, int left_edge_index, int right_edge_index) {
    auto [l,r] = pulley_left_right_mean(z,left_edge_index,right_edge_index);
    auto gradient  = (r - l) / (double)z.size();
    return gradient;
  }

  std::function<int(int,int)> mk_edge_adjust(int left_edge_index_previous, int width_n) {

    return [=](int left_edge_index, int right_edge_index) mutable {
      auto right_edge_index_previous = left_edge_index_previous + width_n;

      double edge_adjust = right_edge_index - left_edge_index - width_n;
      
      auto denominator = (std::abs(left_edge_index - left_edge_index_previous)  + std::abs(right_edge_index - right_edge_index_previous));

      if(denominator != 0) {
        auto proportion = std::abs(left_edge_index - left_edge_index_previous) / denominator;
        left_edge_index_previous -= (int)edge_adjust * proportion;

        if(left_edge_index_previous < 0) left_edge_index_previous = 0;
      }

      return left_edge_index_previous;
    };

  }

  bool operator==(const profile &a, const profile &b)
  {
    auto tx = a.x_off == b.x_off;
    auto ty = a.y == b.y;
    auto tz = std::ranges::equal(a.z, b.z);
    return tx && ty && tz;
  }

  z_type trim_nan(const z_type& z) {

    auto left = std::find_if(z.begin(), z.end(), [](z_element z){ return !std::isnan(z); });
    auto right = std::find_if(z.rbegin(), z.rend(), [](z_element z){ return !std::isnan(z); });

    return {left,right.base()};
  }
  
  void constraint_substitute(z_type& z, z_element z_min, z_element z_max, z_element sub) {
    for (auto &e : z)
    {
      if(!std::isnan(e)) {
        if (e < z_min || e > z_max) {
          e = sub;
        }
      }
    } 
  }

  std::function<profile(profile)> mk_adjust_y(const cads::z_element ref)
  {
    return [=](cads::profile p) 
    {
      p.y = 0;
      return p;
    };
  }

} // namespace cads