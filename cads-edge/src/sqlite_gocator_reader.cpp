#include <sqlite_gocator_reader.h>

#include <algorithm>
#include <ios>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <thread>
#include <vector>
#include <cmath>

#include <spdlog/spdlog.h>
#include <db.h>
#include <nlohmann/json.hpp>

using namespace std;
using namespace std::chrono;

using json = nlohmann::json;

extern json global_config;

namespace cads
{

  bool SqliteGocatorReader::Start_impl()
  {
    if (m_stopped)
    {
      m_stopped = false;
      m_thread = std::jthread{&SqliteGocatorReader::OnData, this};
    }

    return false;
  }

  void SqliteGocatorReader::Stop_impl()
  {
    if (!m_stopped)
    {
      m_stopped = true;
      m_thread.join();
    }
  }

  SqliteGocatorReader::SqliteGocatorReader(Io &gocatorFifo) : GocatorReaderBase(gocatorFifo) {}

  SqliteGocatorReader::~SqliteGocatorReader() {
    Stop_impl();
  }

  void SqliteGocatorReader::OnData()
  {
    using namespace std::chrono_literals;

    auto data_src = global_config["data_source"].get<std::string>();
    auto [params, err2] = fetch_profile_parameters(data_src);

    m_gocatorFifo.enqueue({msgid::gocator_properties, params});

    double y_resolution = 1000 * global_conveyor_parameters.TypicalSpeed / constants_gocator.Fps;
    auto pulley_period = 1000000us / (int)sqlite_gocator_config.fps;
    double cnt = 0;
    auto current_time = std::chrono::high_resolution_clock::now();

    do
    {
      auto fetch_profile = fetch_belt_coro(0,std::get<1>(sqlite_gocator_config.range), std::get<0>(sqlite_gocator_config.range), 256, data_src);

      while (!m_stopped)
      {

        auto [co_terminate, cv] = fetch_profile.resume(0);
        auto [idx, p] = cv;

        if (co_terminate && !sqlite_gocator_config.forever)
        {
          m_stopped = true;
          break;
        }else if(co_terminate && sqlite_gocator_config.forever) {
          break;
        }

        // Trim NaN's
        auto first = std::find_if(p.z.begin(), p.z.end(), [](z_element z)
                                  { return !std::isnan(z); });
        auto last = std::find_if(p.z.rbegin(), p.z.rend(), [](z_element z)
                                 { return !std::isnan(z); });

        if(!m_stopped) m_gocatorFifo.enqueue({msgid::scan, profile{current_time,cnt*y_resolution, p.x_off, z_type(first, last.base())}});

        current_time += pulley_period;
        cnt++;
                     
        if (m_gocatorFifo.size_approx() > buffer_warning_increment)
        {
          //spin lock
          while(m_gocatorFifo.size_approx() > buffer_warning_increment / 4){}
        }

      }
    } while (sqlite_gocator_config.forever && !m_stopped);
    
    m_gocatorFifo.enqueue({msgid::finished, 0});
  }

}
