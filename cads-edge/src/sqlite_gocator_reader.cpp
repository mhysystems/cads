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
      m_thread = std::thread{&SqliteGocatorReader::OnData, this};
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

  SqliteGocatorReader::~SqliteGocatorReader()
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

    sqlite3 *db = nullptr;
    sqlite3_stmt *stmt = nullptr;

    int err = sqlite3_open_v2(data_src.c_str(), &db, SQLITE_OPEN_READONLY, nullptr);
    auto query = R"(SELECT * FROM PROFILE ORDER BY Y)"s;
    err = sqlite3_prepare_v2(db, query.c_str(), query.size(), &stmt, NULL);

    while (m_loop)
    {

      err = sqlite3_step(stmt);

      decltype(profile::y) y;
      if (err == SQLITE_ROW)
      {

        if constexpr (std::is_same_v<decltype(y), double>)
        {
          y = sqlite3_column_double(stmt, 0);
        }
        else
        {
          y = (decltype(y))sqlite3_column_int64(stmt, 0);
        }
        double x_off = sqlite3_column_double(stmt, 1);
        z_element *z = (z_element *)sqlite3_column_blob(stmt, 2);
        int len = sqlite3_column_bytes(stmt, 2) / sizeof(*z);
        
        m_gocatorFifo.enqueue({msgid::scan,profile{y, x_off, {z, z + len}}});
      }
      else
      {
        m_gocatorFifo.enqueue({msgid::finished,0});
        m_loop = false;
      }
    }

    sqlite3_finalize(stmt);
    sqlite3_close(db);
  }

}
