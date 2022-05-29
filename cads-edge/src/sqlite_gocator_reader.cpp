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
    auto [yResolution, xResolution, zResolution, zOffset, encoderResolution] = fetch_profile_parameters(data_src);
    m_gocatorFifo.enqueue({msgid::resolutions,fetch_profile_parameters(data_src)});
    
    m_yResolution = yResolution;
    m_xResolution = xResolution;
    m_zResolution = zResolution;
    m_zOffset = zOffset;
    m_encoder_resolution = encoderResolution;

    {
      unique_lock<mutex> lock(m_mutex);
      m_condition.notify_all();
    }

    sqlite3 *db = nullptr;
    sqlite3_stmt *stmt = nullptr;

    int err = sqlite3_open_v2(data_src.c_str(), &db, SQLITE_OPEN_READONLY, nullptr);
    auto query = R"(SELECT * FROM PROFILE ORDER BY Y)"s;
    err = sqlite3_prepare_v2(db, query.c_str(), query.size(), &stmt, NULL);

    auto start = std::chrono::high_resolution_clock::now();
    uint64_t cnt = 0;
    while (m_loop)
    {
      ++cnt;
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

    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
    spdlog::info("CNT: {}, DUR: {}, RATE(ms):{} ", cnt, duration, (double)cnt / duration);

    sqlite3_finalize(stmt);
    sqlite3_close(db);
  }

}
