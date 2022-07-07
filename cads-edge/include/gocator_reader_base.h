#pragma once

#include <condition_variable>
#include <atomic>
#include <mutex>
#include <tuple>

#include <GoSdk/GoSdkDef.h>

#include <readerwriterqueue.h>
#include <msg.h>
#include <profile.h>
#include <constants.h>


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
  std::atomic<double> m_yResolution = 1.0;
  std::atomic<double> m_encoder_resolution = 1.0;
  std::condition_variable m_condition;
  std::mutex m_mutex;
  std::atomic<bool> m_loop;
  std::atomic<bool> m_first_frame = true;
  static std::atomic<bool> terminate;
  
  z_type k16sToFloat(k16s*, k16s*, double, double);
  static void sigint_handler(int s);

public:

  virtual void RunForever() = 0;
	virtual void Start() = 0;
	virtual void Stop() = 0;

	GocatorReaderBase(moodycamel::BlockingReaderWriterQueue<msg>&);
  virtual ~GocatorReaderBase() = default;
};


}

