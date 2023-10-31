#pragma once

#include <condition_variable>
#include <atomic>
#include <mutex>
#include <tuple>

#include <GoSdk/GoSdkDef.h>

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
  virtual bool Start_impl();
  virtual bool Align_impl();
  virtual void Stop_impl(bool);
  virtual bool SetFrameRate(double);
  virtual bool SetFoV_impl(double);
  virtual bool ResetAlign_impl();

protected:
	Io<msg>& m_gocatorFifo;
  std::atomic<bool> m_stopped = true;
  std::atomic<bool> m_first_frame = true;
  
  z_type k16sToFloat(k16s*, k16s*, double, double);

public:
  bool Start(double);
	void Stop(bool);
  bool Align();
  bool SetFoV(double);
  bool ResetAlign();

	GocatorReaderBase(Io<msg>&);
  virtual ~GocatorReaderBase() = default;
};


}

