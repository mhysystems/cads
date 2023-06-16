#pragma once

#include <string>
#include <thread>

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wall"

#include <GoSdk/GoSensor.h>
#include <GoSdk/GoSystem.h>
#include <kApi/kApiDef.h>

#pragma GCC diagnostic pop
#include "gocator_reader_base.h"

namespace cads
{

class GocatorReader : public GocatorReaderBase
{

	GocatorReader() = delete;
	GocatorReader(const GocatorReader&) = delete;
	GocatorReader& operator=(const GocatorReader&) = delete;
	GocatorReader(GocatorReader&&) = delete;
	GocatorReader& operator=(GocatorReader&&) = delete;

protected:
	
	void* m_system;
	void* m_assembly;
	void* m_sensor;
  bool m_trim = true;
  std::atomic<size_t> m_buffer_size_warning = 4096;
  
	static kStatus OnData(kPointer context, GoSensor sensor, GoDataSet dataset);
  static kStatus OnSystem(kPointer context, GoSystem system, GoDataSet data);
	virtual kStatus OnData(GoSensor sensor, GoDataSet dataset);
  virtual kStatus OnSystem(GoSystem system, GoDataSet dataset);

public:

	void Start();
	void Stop();
  void Log();
	GocatorReader(Io&);
	virtual ~GocatorReader();
};

}


