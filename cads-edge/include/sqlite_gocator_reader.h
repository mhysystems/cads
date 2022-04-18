#pragma once

#include <readerwriterqueue.h>
#include <condition_variable>
#include <atomic>
#include <thread>

#include "gocator_reader_base.h"

namespace cads
{

class SqliteGocatorReader : public GocatorReaderBase
{

	SqliteGocatorReader() = delete;
	SqliteGocatorReader(const SqliteGocatorReader&) = delete;
	SqliteGocatorReader& operator=(const SqliteGocatorReader&) = delete;
	SqliteGocatorReader(SqliteGocatorReader&&) = delete;
	SqliteGocatorReader& operator=(SqliteGocatorReader&&) = delete;

protected:
	std::atomic<bool> m_loop = false;
  std::thread m_thread;
	double m_yResolution = 0.0;
	
  void OnData();

public:

  void RunForever();
	void Start();
	void Stop();
	SqliteGocatorReader(moodycamel::BlockingReaderWriterQueue<char>&);
	~SqliteGocatorReader();
};


}

