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

  void SqliteGocatorReader::Start()
  {
    if (m_stopped)
    {
      m_stopped = false;
      m_thread = std::jthread{&SqliteGocatorReader::OnData, this};
    }
  }

  void SqliteGocatorReader::Stop()
  {
    if (!m_stopped)
    {
      m_stopped = true;
      m_thread.join();
    }
  }

  SqliteGocatorReader::SqliteGocatorReader(moodycamel::BlockingReaderWriterQueue<msg> &gocatorFifo, SqliteGocatorConfig config) : GocatorReaderBase(gocatorFifo), m_config(config)
  {
  }

  SqliteGocatorReader::SqliteGocatorReader(moodycamel::BlockingReaderWriterQueue<msg> &gocatorFifo) : GocatorReaderBase(gocatorFifo), m_config(sqlite_gocator_config)
  {
  }

  void SqliteGocatorReader::OnData()
  {
    auto data_src = global_config["data_source"].get<std::string>();
    auto [params, err2] = fetch_profile_parameters(data_src);

    m_gocatorFifo.enqueue({msgid::gocator_properties, GocatorProperties{params.y_res, params.x_res, params.z_res, params.z_off, params.encoder_res, m_config.fps}});

    m_yResolution = params.y_res;
    m_encoder_resolution = params.encoder_res;
    auto pulley_period_us = 1000000 / m_config.fps;
    uint64_t cnt = 0;

    do
    {
      auto fetch_profile = fetch_belt_coro(0,std::get<1>(m_config.range), std::get<0>(m_config.range), 256, data_src);
      auto loop_time = std::chrono::high_resolution_clock::now();
      std::chrono::duration<double, std::micro> slept_for(0);
      while (!m_stopped)
      {

        auto [co_terminate, cv] = fetch_profile.resume(0);
        auto [idx, p] = cv;

        if (co_terminate && !m_config.forever)
        {
          m_gocatorFifo.enqueue({msgid::finished, 0});
          m_stopped = true;
          break;
        }else if(co_terminate && m_config.forever) {
          break;
        }

        // Trim NaN's
        auto first = std::find_if(p.z.begin(), p.z.end(), [](z_element z)
                                  { return !std::isnan(z); });
        auto last = std::find_if(p.z.rbegin(), p.z.rend(), [](z_element z)
                                 { return !std::isnan(z); });

        m_gocatorFifo.enqueue({msgid::scan, profile{p.y, p.x_off, z_type(first, last.base())}});
        
        auto now = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double, std::micro> dt = now - loop_time;

        loop_time = now;
        cnt++;
        auto sleep_for = uint64_t(2*pulley_period_us - (dt + slept_for).count() - m_config.delay);
        
        auto start = std::chrono::system_clock::now();
        std::this_thread::sleep_for(std::chrono::microseconds(sleep_for));
        auto end = std::chrono::system_clock::now();  
        slept_for = end - start;           
                     
        if (m_gocatorFifo.size_approx() > buffer_warning_increment)
        {
          //spin lock
          while(m_gocatorFifo.size_approx() > buffer_warning_increment / 4){}
        }

        if (terminate)
        {
          m_gocatorFifo.enqueue({msgid::finished, 0});
          m_stopped = true;
        }
      }
    } while (m_config.forever && !terminate);
  }

}
