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
#include <io.hpp>

namespace cads
{

class GocatorReaderBase
{

	GocatorReaderBase() = delete;
	GocatorReaderBase(const GocatorReaderBase&) = delete;
	GocatorReaderBase& operator=(const GocatorReaderBase&) = delete;
	GocatorReaderBase(GocatorReaderBase&&) = delete;
	GocatorReaderBase& operator=(GocatorReaderBase&&) = delete;
  virtual bool Start_impl(double);

protected:
	Io& m_gocatorFifo;
  std::atomic<bool> m_stopped = true;
  std::atomic<bool> m_first_frame = true;
  
  z_type k16sToFloat(k16s*, k16s*, double, double);

public:
	virtual void Start() = 0;
  bool Start(double);
	virtual void Stop() = 0;

	GocatorReaderBase(Io&);
  virtual ~GocatorReaderBase() = default;
};


}

