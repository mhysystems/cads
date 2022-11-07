#include <chrono>
#include <cmath>
#include <ranges>

#include <spdlog/spdlog.h>
#include <boost/sml.hpp>

#include <belt.h>
#include <filters.h>
#include <intermessage.h>
#include <constants.h>
#include <profile.h>
#include <regression.h>
#include <edge_detection.h>

namespace cads
{

  std::function<std::tuple<bool, double>(double)> mk_pulley_revolution()
  {

    auto pully_circumfrence = global_config["pulley_circumfrence"].get<double>(); // In mm
    auto trigger_distance = pully_circumfrence / 2;
    double schmitt1 = 1.0, schmitt0 = -1.0;
    auto schmitt_trigger = mk_schmitt_trigger(0.001f);

    return [=](double pulley_height) mutable -> std::tuple<bool, double>
    {
      auto rtn = std::make_tuple(false, trigger_distance);
      schmitt1 = schmitt_trigger(pulley_height);
      if ((std::signbit(schmitt1) == false && std::signbit(schmitt0) == true) || (std::signbit(schmitt1) == true && std::signbit(schmitt0) == false))
      {
        std::get<0>(rtn) = true;
      }

      schmitt0 = schmitt1;
      return rtn;
    };
  }

  coro<int, profile> encoder_distance_estimation(std::function<void(void)> next)
  {
    namespace sml = boost::sml;

    profile p;
    std::deque<profile> fifo;
    auto pulley_revolution = mk_pulley_revolution();

    struct root_event 
    {
      double distance = 0.0;
      profile p;
    };

    struct profile_event
    {
      profile p;
    };

    class transitions
    {
    public:
      auto operator()() const noexcept
      {
        using namespace sml;
        std::deque<profile> fifo;

        auto take_action = [=](const root_event& o) mutable {
          fifo.push_back(o.p);
        };

        return make_transition_table(
            *"drop"_s + event<root_event> / [](const root_event & e) {process(profile_event{e.p});} = "take"_s,
             "take"_s + event<profile_event> / [&](const profile_event & e) mutable {fifo.push_back(e.p);} = "take"_s,
             "take"_s + event<root_event>  = "drain"_s,
             "drain"_s + event<origin> [guard] = "take"_s
        );
      }
    };

    sml::sm<transitions> sm;

    for (auto terminate = true; terminate;)
    {
      std::tie(p, terminate) = co_yield 0;
      if (terminate)
        continue;

      auto [a, b] = pulley_revolution(0);

      sm.process_event(origin{b,a,p});

      fifo.push_back(p);

      auto step_size = b / fifo.size();
      for (auto i = 0; i < fifo.size(); i++)
      {
        auto e = fifo[i];
        e.y = distance + i * step_size;
        next();
      }
      distance += b;
      fifo.clear();
    }
  }

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
      amplitude_extraction(bottom,false);

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

  std::function<int(z_type &, int, int)> mk_profiles_align(int width_n)
  {
    namespace sr = std::ranges;

    z_type prev_z;

    return [=](z_type &z, int left_edge_index, int right_edge_index) mutable
    {
      if (prev_z.size() != 0)
      {

        auto dbg = correlation_lowest(prev_z, z | sr::views::take(right_edge_index) | sr::views::drop(left_edge_index));
        left_edge_index -= dbg;
        auto f = z | sr::views::take(left_edge_index + width_n) | sr::views::drop(left_edge_index);
        prev_z = {f.begin(), f.end()};
        return left_edge_index;
      }
      else
      {
        // auto f = z | sr::views::take(left_edge_index + width_n) | sr::views::drop(left_edge_index);
        // prev_z = {f.begin(), f.end()};
        return left_edge_index;
      }
    };
  }

  std::function<double(double)> mk_differentiation(double v0)
  {

    return [=](double v1) mutable
    {
      auto dv = (v1 - v0) * 64;
      v0 = v1;
      return dv;
    };
  }
}