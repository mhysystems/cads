#pragma once

#include <readerwriterqueue.h>
#include <condition_variable>
#include <atomic>
#include <thread>

#include "gocator_reader_base.h"
#include <constants.h>

namespace cads
{

  struct SqliteGocatorConfig {
    using range_type = std::tuple<long long,long long>;
    range_type Range;
    double Fps;
    bool Forever;
    double Delay;
    std::filesystem::path Source;
    double TypicalSpeed;
  };

  class SqliteGocatorReader : public GocatorReaderBase
  {

    SqliteGocatorReader() = delete;
    SqliteGocatorReader(const SqliteGocatorReader&) = delete;
    SqliteGocatorReader& operator=(const SqliteGocatorReader&) = delete;
    SqliteGocatorReader(SqliteGocatorReader&&) = delete;
    SqliteGocatorReader& operator=(SqliteGocatorReader&&) = delete;
    virtual bool Start_impl();
    virtual void Stop_impl();

  protected:
    std::atomic<bool> m_loop = false;
    std::jthread m_thread;
    SqliteGocatorConfig config;

    void OnData();

  public:

    SqliteGocatorReader(SqliteGocatorConfig,Io&);
    virtual ~SqliteGocatorReader();
  };


}

