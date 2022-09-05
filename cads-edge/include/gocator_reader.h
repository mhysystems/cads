#pragma once

#include <string>

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
  bool m_use_encoder = true;
  std::atomic<k64s> m_yOffset = 0;
  std::atomic<size_t> m_buffer_size_warning = 4096;
  
	static kStatus OnData(kPointer context, GoSensor sensor, GoDataSet dataset);
  static kStatus OnSystem(kPointer context, GoSystem system, GoDataSet data);
	virtual kStatus OnData(GoSensor sensor, GoDataSet dataset);
  virtual kStatus OnSystem(GoSystem system, GoDataSet dataset);

public:

  void RunForever();
	void Start();
	void Stop();
	GocatorReader(moodycamel::BlockingReaderWriterQueue<cads::msg>&, std::string ip_add = "");
  GocatorReader(moodycamel::BlockingReaderWriterQueue<cads::msg>&, bool, std::string ip_add = "");
	virtual ~GocatorReader();
};

}


