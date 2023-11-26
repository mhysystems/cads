#include <chrono>
#include <cmath>
#include <ranges>
#include <functional>

#include <spdlog/spdlog.h>
#include <boost/sml.hpp>

#include <belt.h>
#include <filters.h>
#include <constants.h>
#include <profile_t.h>
#include <regression.h>
#include <edge_detection.h>
#include <utils.hpp>

namespace
{
  double pulley_speed_adjustment(double milliseconds, double T0, double T1)
  {

    auto m = 1.0 / (T1 - T0);

    if (milliseconds >= T0)
    {
      const auto period = milliseconds - T0;
      auto c = 1 - m * period;
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

std::function<PulleyRevolution(double)> 
  mk_pseudo_revolution(double trigger_distance)
  {
    double next_revolution = trigger_distance;

    return [=](double len) mutable -> PulleyRevolution
    {
      auto rtn = PulleyRevolution{false, trigger_distance};

      if (len > next_revolution)
      {
        next_revolution = len + trigger_distance;
        std::get<0>(rtn) = true;
      }

      return rtn;
    };
  }



 std::function<PulleyRevolution(double)> 
  mk_pulley_revolution(RevolutionSensorConfig config)
  {
    auto bidirectional = config.bidirectional;
    auto bias = config.bias;
    auto trigger_distance = config.trigger_distance;
    auto cnt_est_threshold = config.skip; //period * constants_gocator.Fps * 0.9;

    double schmitt1 = 1.0, schmitt0 = -1.0;
    auto schmitt_trigger = mk_schmitt_trigger(config.threshold);

    decltype(cnt_est_threshold) cnt = 0;
    auto avg_max_fn = mk_online_mean(bias);
    auto avg_min_fn = mk_online_mean(bias);

    auto avg_max = bias;
    auto avg_min = bias;

    return [=](double pulley_height) mutable -> PulleyRevolution
    {
      cnt++;


      auto max = std::max(avg_max,pulley_height);
      auto min = std::min(avg_min,pulley_height);


      // Keep avg falling to middle of min max values so avg max/min 
      // don't get stuck at a particular value.
      max = max * (1 + std::signbit(avg_max) * 0.00001);
      min = min * (1 - std::signbit(avg_min) * 0.00001);
      avg_max = avg_max_fn(max);
      avg_min = avg_min_fn(min);
      bias = avg_min * 0.75 + avg_max * (1 - 0.75);
      
      auto rtn = PulleyRevolution{false, trigger_distance};

      schmitt1 = schmitt_trigger(pulley_height,bias);

      if ((cnt > cnt_est_threshold) && ((std::signbit(schmitt1) == true && std::signbit(schmitt0) == false) || 
         (bidirectional && (std::signbit(schmitt1) == false && std::signbit(schmitt0) == true))))
      {
        cnt = 0;
        std::get<0>(rtn) = true;
      }

      schmitt0 = schmitt1;
      return rtn;
    };
  }

  coro<cads::msg, cads::msg,1> encoder_distance_estimation(cads::Io<msg> &next, double stride)
  {
    namespace sml = boost::sml;

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
      double stride;
      std::deque<profile> fifo;
      decltype(next) csp;
      global_t(decltype(next) i) : csp(i){}
    } global(next);

    global.stride = stride;

    class transitions
    {
    public:
      auto operator()() const noexcept
      {
        using namespace sml;

        auto drain_action = [](global_t &global, const root_event &o) mutable
        {
          auto n = std::size_t(o.root_distance / global.stride);
          auto step_size = global.fifo.size() / n;

          for(size_t i = 0; i < n; ++i) 
          {
            auto e = global.fifo[std::size_t(i*step_size)];
            e.y = global.distance + i * global.stride;
            global.csp.enqueue({msgid::scan,e});
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
    msg args_in;

    for (auto terminate = false; !terminate;)
    {
      std::tie(args_in, terminate) = co_yield {msgid::nothing,0};
      
      if (terminate)
        continue;

      auto id = std::get<0>(args_in);

      switch(id) {
        case msgid::pulley_revolution_scan: {
          auto [root, root_distance,p] = std::get<PulleyRevolutionScan>(std::get<1>(args_in));
          if (root)
          {
            sm.process_event(root_event{root_distance, p});
          }
          else
          {
            sm.process_event(profile_event{p});
          }
          break;
        }
        case msgid::stopped:
        case msgid::finished: {
          next.enqueue(args_in);
          terminate = true;
          break;
        }
        default:
          next.enqueue(args_in);
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


 
  std::function<std::tuple<double,double,double>(PulleyRevolution,double,std::chrono::time_point<std::chrono::system_clock>)> mk_pulley_stats(double avg_speed, double pulley_circumfrence)
  {
    using namespace std::placeholders;

    auto T0 = pulley_circumfrence / avg_speed; // in milliseconds
    auto T1 = 6 * T0;  // in milliseconds
    auto barrel_origin_time = std::chrono::high_resolution_clock::now();
    
    double period = 1.0;
    long cnt = 0;
    auto adjust = std::bind(pulley_speed_adjustment, _1, T0, T1);
    auto amplitude_extraction = mk_amplitude_extraction();

    double speed = avg_speed; // m/s
    double amplitude = 0;
    double frequency = 0;

    return [=](PulleyRevolution pulley_revolution, double pulley_osc, std::chrono::time_point<std::chrono::system_clock> now) mutable -> std::tuple<double,double,double>
    {
      auto [root, root_distance] = pulley_revolution;

      std::chrono::duration<double, std::milli> dt = now - barrel_origin_time;
      period = dt.count();
      amplitude_extraction(pulley_osc, false);

      if (root && cnt > 0)
      {
        amplitude = amplitude_extraction(pulley_osc, true);
        speed = root_distance / period;
        frequency = ((root_distance / pulley_circumfrence ) * 1000.0) / ((double)period);
        
        barrel_origin_time = now;
        cnt++;
      }

      if (cnt == 0)
      {
        barrel_origin_time = now;
        cnt++;
      }

      auto speed_mask = adjust(period);
      return {speed * (cnt > 1 ? speed_mask: 1.0), frequency, amplitude};
    };
  }
}