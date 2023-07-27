#pragma once

#include <readerwriterqueue.h>
#include <condition_variable>
#include <atomic>
#include <thread>

#include "gocator_reader_base.h"
#include <constants.h>

namespace cads
{

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

