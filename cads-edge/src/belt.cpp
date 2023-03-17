#include <chrono>
#include <cmath>
#include <ranges>
#include <functional>

#include <spdlog/spdlog.h>
#include <boost/sml.hpp>

#include <belt.h>
#include <filters.h>
#include <intermessage.h>
#include <constants.h>
#include <profile.h>
#include <regression.h>
#include <edge_detection.h>

namespace
{
  double pulley_speed_adjustment(double milliseconds, double T0, double T1)
  {

    auto m = 1.0 / (T1 - T0);

    if (milliseconds >= T0)
    {
      auto c = -m * (milliseconds) + (1 + m * T0);
      if (c > 0.0)
      {
        return c;
      }
      else
      {
        return 0.0;
      }
    }
    else
    {
      return 1.0;
    }
  }
}

namespace cads
{

  std::function<std::tuple<bool, double>(double)> mk_pulley_revolution()
  {

    auto pully_circumfrence = global_conveyor_parameters.PulleyCircumference; // In mm
    auto trigger_distance = pully_circumfrence / 2;
    double schmitt1 = 1.0, schmitt0 = -1.0;
    auto schmitt_trigger = mk_schmitt_trigger();

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

  std::function<PulleyRevolution(double)> 
  mk_pulley_revolution2()
  {
    auto n = revolution_sensor_config.trigger_num;
    auto bidirectional = revolution_sensor_config.bidirectional;
    auto bias = revolution_sensor_config.bias;
    auto pully_circumfrence = global_conveyor_parameters.PulleyCircumference; // In mm
    auto trigger_distance = pully_circumfrence / n;
    double schmitt1 = 1.0, schmitt0 = -1.0;
    auto schmitt_trigger = mk_schmitt_trigger(bias);
    auto time0 = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> dt = time0 - time0;

    return [=](double pulley_height) mutable -> PulleyRevolution
    {
      auto rtn = PulleyRevolution{false, trigger_distance,dt};
      schmitt1 = schmitt_trigger(pulley_height);
      if ((std::signbit(schmitt1) == true && std::signbit(schmitt0) == false) || 
         (bidirectional && (std::signbit(schmitt1) == false && std::signbit(schmitt0) == true)))
      {
        auto now = std::chrono::high_resolution_clock::now();
        dt = now - time0;
        time0 = now;
        std::get<0>(rtn) = true;
        std::get<2>(rtn) = dt;
      }

      schmitt0 = schmitt1;
      return rtn;
    };
  }

  coro<int, std::tuple<PulleyRevolution, profile>> encoder_distance_estimation(std::function<void(profile)> next)
  {
    namespace sml = boost::sml;

    auto args_in = std::tuple<PulleyRevolution, profile>{};
    std::deque<profile> fifo;

    struct root_event
    {
      double root_distance;
      profile p;
    };

    struct profile_event
    {
      profile p;
    };

    struct global_t
    {
      double distance = 0;
      std::deque<profile> fifo;
      decltype(next) csp;
    } global;

    global.csp = next;

    class transitions
    {
    public:
      auto operator()() const noexcept
      {
        using namespace sml;

        auto drain_action = [](global_t &global, const root_event &o) mutable
        {
          auto step_size = o.root_distance / global.fifo.size();
          for (std::size_t i = 0; i < global.fifo.size(); i++)
          {
            auto e = global.fifo[i];
            e.y = global.distance + i * step_size;
            global.csp(e);
          }

          global.distance += o.root_distance;
          global.fifo.clear();
          process(profile_event{o.p});
        };

        auto init_action = [](const root_event &e)
        {
          process(profile_event{e.p});
        };

        return make_transition_table(
            *"drop"_s + event<root_event> / init_action = "take"_s,
            "take"_s + event<profile_event> / [](global_t &global, const profile_event &e) mutable
                           { global.fifo.push_back(e.p); } = "take"_s,
            "take"_s + event<root_event> / drain_action = "take"_s);
      }
    };

    sml::sm<transitions> sm{global};

    for (auto terminate = false; !terminate;)
    {
      std::tie(args_in, terminate) = co_yield 0;
      auto [pulley_revolution, p] = args_in;

      if (terminate)
        continue;

      auto [root, root_distance, time] = pulley_revolution;

      if (root)
      {
        sm.process_event(root_event{root_distance, p});
      }
      else
      {
        sm.process_event(profile_event{p});
      }
    }
  }

coro<int, std::tuple<double, profile>> encoder_distance_estimation2(std::function<void(profile)> next)
  {
    namespace sml = boost::sml;

    auto args_in = std::tuple<double, profile>{};
    std::deque<profile> fifo;
    auto pulley_revolution = mk_pulley_revolution();

    struct root_event
    {
      double root_distance;
      profile p;
    };

    struct profile_event
    {
      profile p;
    };

    struct global_t
    {
      double distance = 0;
      std::deque<profile> fifo;
      decltype(next) csp;
    } global;

    global.csp = next;

    class transitions
    {
    public:
      auto operator()() const noexcept
      {
        using namespace sml;

        auto drain_action = [](global_t &global, const root_event &o) mutable
        {
          auto step_size = o.root_distance / global.fifo.size();
          for (std::size_t i = 0; i < global.fifo.size(); i++)
          {
            auto e = global.fifo[i];
            e.y = global.distance + i * step_size;
            global.csp(e);
          }

          global.distance += o.root_distance;
          global.fifo.clear();
          process(profile_event{o.p});
        };

        auto init_action = [](const root_event &e)
        {
          process(profile_event{e.p});
        };

        return make_transition_table(
            *"drop"_s + event<root_event> / init_action = "take"_s,
            "take"_s + event<profile_event> / [](global_t &global, const profile_event &e) mutable
                           { global.fifo.push_back(e.p); } = "take"_s,
            "take"_s + event<root_event> / drain_action = "take"_s);
      }
    };

    sml::sm<transitions> sm{global};

    for (auto terminate = false; !terminate;)
    {
      std::tie(args_in, terminate) = co_yield 0;
      auto [pully_height, p] = args_in;

      if (terminate)
        continue;

      auto [root, root_distance] = pulley_revolution(pully_height);

      if (root)
      {
        sm.process_event(root_event{root_distance, p});
      }
      else
      {
        sm.process_event(profile_event{p});
      }
    }
  }

  coro<int, std::tuple<double, profile>> encoder_distance_id(std::function<void(profile)> next)
  {

    auto args_in = std::tuple<double, profile>{};

    for (auto terminate = false; !terminate;)
    {
      std::tie(args_in, terminate) = co_yield 0;
      auto [pully_height, p] = args_in;

      if (terminate)
        continue;

      next(p);
    }
  }


 
  std::function<double(PulleyRevolution,double)> mk_pulley_speed(double init)
  {
    using namespace std::placeholders;

    auto pulley_circumfrence = global_conveyor_parameters.PulleyCircumference;
    auto avg_speed = (std::get<0>(global_constraints.SurfaceSpeed) + std::get<0>(global_constraints.SurfaceSpeed)) / 2;
    auto T0 = pulley_circumfrence / avg_speed; // in ms
    auto T1 = 2 * T0;                                                     // in ms
    auto barrel_origin_time = std::chrono::high_resolution_clock::now();
    double speed = init;
    double period = 1.0;
    long cnt = 0;
    auto adjust = std::bind(pulley_speed_adjustment, _1, T0, T1);
    auto amplitude_extraction = mk_amplitude_extraction();

    return [=](PulleyRevolution pulley_revolution, double pulley_osc) mutable -> double
    {
      auto [root, root_distance, root_dt] = pulley_revolution;

      auto now = std::chrono::high_resolution_clock::now();
      std::chrono::duration<double, std::milli> dt = now - barrel_origin_time;
      period = dt.count();
      amplitude_extraction(pulley_osc, false);

      if (root && cnt > 0)
      {
        auto amp = amplitude_extraction(pulley_osc, true);
        speed = root_distance / period;
        barrel_origin_time = now;
        cnt++;

        if (cnt % 50 == 0)
        {
           spdlog::get("cads")->debug("Amplitude: {}",amp);
          spdlog::get("cads")->debug("Barrel Frequency2(Hz): {}", ((root_distance / pulley_circumfrence ) * 1000.0) / ((double)period));
          spdlog::get("cads")->debug("Surface Speed2(m/s): {},{}", speed, speed * adjust(period));
        }
      }

      if (cnt == 0)
      {
        barrel_origin_time = std::chrono::high_resolution_clock::now();
        cnt++;
      }

      auto speed_mask = adjust(period);
      return speed * (cnt > 1 ? speed_mask: 1.0);
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
        auto f = z | sr::views::take(left_edge_index + width_n) | sr::views::drop(left_edge_index);
        prev_z = {f.begin(), f.end()};
        return left_edge_index;
      }
    };
  }

}