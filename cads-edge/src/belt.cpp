#include <chrono>
#include <cmath>
#include <ranges>

#include <spdlog/spdlog.h>

#include <filters.h>
#include <intermessage.h>
#include <constants.h>
#include <profile.h>
#include <regression.h>
#include <edge_detection.h>

namespace cads
{

  std::function<long(double)> mk_pulley_frequency()
  {

    auto barrel_origin_time = std::chrono::high_resolution_clock::now();
    auto pully_circumfrence = global_config["pulley_circumfrence"].get<double>(); // In mm
    double schmitt1 = 1.0, schmitt0 = -1.0;
    auto amplitude_extraction = mk_amplitude_extraction();
    auto schmitt_trigger = mk_schmitt_trigger(0.001f);
    long barrel_cnt = 0;

    return [=](double bottom) mutable -> long
    {
      schmitt1 = schmitt_trigger(bottom);
      if ((std::signbit(schmitt1) == false && std::signbit(schmitt0) == true) || (std::signbit(schmitt1) == true && std::signbit(schmitt0) == false))
      {
        auto now = std::chrono::high_resolution_clock::now();
        auto period = std::chrono::duration_cast<std::chrono::milliseconds>(now - barrel_origin_time).count();
        if (barrel_cnt % 100 == 0)
        {
          spdlog::get("cads")->info("Barrel Frequency(Hz): {}", 1000.0 / ((double)period * 2));
          publish_PulleyOscillation(amplitude_extraction(bottom, true));
          publish_SurfaceSpeed(pully_circumfrence / (2 * period));
        }

        barrel_cnt++;
        barrel_origin_time = now;
      }

      schmitt0 = schmitt1;
      return barrel_cnt;
    };
  }

  std::function<int(z_type &,int,int)> mk_profiles_align(int width_n) {
    namespace sr = std::ranges;

    z_type prev_z;

    return [=](z_type &z,int left_edge_index, int right_edge_index) mutable {
      if(prev_z.size() != 0) {

        auto dbg = correlation_lowest(prev_z, z | sr::views::take(right_edge_index) | sr::views::drop(left_edge_index));
        left_edge_index -= dbg;
        auto f = z | sr::views::take(left_edge_index + width_n) | sr::views::drop(left_edge_index);
        prev_z = {f.begin(), f.end()};
        return left_edge_index;
      }else {
        //auto f = z | sr::views::take(left_edge_index + width_n) | sr::views::drop(left_edge_index);
        //prev_z = {f.begin(), f.end()};
        return left_edge_index;
      }
    
    };
  }

  std::function<double(double)> mk_differentiation(double v0) {

    return [=](double v1) mutable {
      auto dv = (v1 - v0) * 64;
      v0 = v1;
      return dv;    
    };
  }
}