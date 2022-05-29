#pragma once

#include <readerwriterqueue.h>
#include <profile.h>
#include <msg.h>

#include <condition_variable>
#include <atomic>
#include <mutex>
#include <tuple>

namespace cads
{

class GocatorReaderBase
{

	GocatorReaderBase() = delete;
	GocatorReaderBase(const GocatorReaderBase&) = delete;
	GocatorReaderBase& operator=(const GocatorReaderBase&) = delete;
	GocatorReaderBase(GocatorReaderBase&&) = delete;
	GocatorReaderBase& operator=(GocatorReaderBase&&) = delete;

protected:
	moodycamel::BlockingReaderWriterQueue<msg>& m_gocatorFifo;
  double m_yResolution = 1.0;
  double m_xResolution = 1.0;
  double m_zResolution = 1.0;
  double m_zOffset = 1.0;
  double m_encoder_resolution = 1.0;
  std::condition_variable m_condition;
  std::mutex m_mutex;
  std::atomic<bool> m_loop;
  std::atomic<bool> m_first_frame = true;

public:

  virtual void RunForever() = 0;
	virtual void Start() = 0;
	virtual void Stop() = 0;
	GocatorReaderBase(moodycamel::BlockingReaderWriterQueue<msg>&);
  virtual resolutions_t get_gocator_constants();

};


}

