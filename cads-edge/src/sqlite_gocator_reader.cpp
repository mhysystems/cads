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
    if (!m_loop)
    {
      m_loop = true;
      m_thread = std::jthread{&SqliteGocatorReader::OnData, this};
    }
  }

  void SqliteGocatorReader::Stop()
  {
    if (m_loop)
    {
      m_loop = false;
      m_thread.join();
    }
  }

  SqliteGocatorReader::SqliteGocatorReader(moodycamel::BlockingReaderWriterQueue<msg> &gocatorFifo) : GocatorReaderBase(gocatorFifo)
  {
  }

  void SqliteGocatorReader::RunForever()
  {
  }

  void SqliteGocatorReader::OnData()
  {
    auto data_src = global_config["data_source"].get<std::string>();
    auto [params, err2] = fetch_profile_parameters(data_src);

    m_gocatorFifo.enqueue({msgid::resolutions, std::tuple<double, double, double, double, double>{params.y_res, params.x_res, params.z_res, params.z_off, params.encoder_res}});

    m_yResolution = params.y_res;
    m_encoder_resolution = params.encoder_res;

    auto fetch_profile = fetch_belt_coro(0, std::numeric_limits<int>::max(), 256, data_src);

    while (m_loop)
    {

      auto [co_terminate, cv] = fetch_profile.resume(0);
      auto [idx, p] = cv;

      if (co_terminate)
      {
        m_gocatorFifo.enqueue({msgid::finished, 0});
        m_loop = false;
        break;
      }
      
      // Trim NaN's
      auto first = std::find_if(p.z.begin(),  p.z.end(),  [](z_element z) {return !std::isnan(z);});
      auto last = std::find_if(p.z.rbegin(), p.z.rend(), [](z_element z) {return !std::isnan(z);});

      m_gocatorFifo.enqueue({msgid::scan, profile{p.y,p.x_off,z_type(first,last.base())}});
      
      if (terminate)
      {
        m_gocatorFifo.enqueue({msgid::finished, 0});
        m_loop = false;
      }
    }
  }

}
