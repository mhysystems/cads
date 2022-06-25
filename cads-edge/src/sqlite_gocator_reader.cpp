#include <sqlite_gocator_reader.h>
#include <z_data_generated.h>
#include <p_config_generated.h>

#include <algorithm>
#include <ios>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <thread>
#include <vector>

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
    auto [yResolution, xResolution, zResolution, zOffset, encoderResolution,err2] = fetch_profile_parameters(data_src);
  
    m_gocatorFifo.enqueue({msgid::resolutions,std::tuple<double, double, double, double, double>{yResolution, xResolution, zResolution, zOffset, encoderResolution}});
    
    m_yResolution = yResolution;
    m_encoder_resolution = encoderResolution;

   auto fetch_profile = fetch_belt_coro(0,std::numeric_limits<short>::max(),data_src);

    while (m_loop)
    {

      auto [co_terminate, cv] = fetch_profile(0);
      auto [idx,p] = cv;

      if (co_terminate) {
        m_gocatorFifo.enqueue({msgid::finished,0});
        m_loop = false;
        break;
      }

      m_gocatorFifo.enqueue({msgid::scan,p});

    }
  }

}
