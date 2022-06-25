#pragma once

#include <GoSdk/GoSensor.h>
#include <GoSdk/GoSystem.h>
#include <kApi/kApiDef.h>

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
	
	GoSystem m_system;
	kAssembly m_assembly;
	GoSensor m_sensor;
  bool m_use_encoder = true;
  std::atomic<k64s> m_yOffset = 0;
  std::atomic<int> m_buffer_size_warning = 1024;
  
	static kStatus OnData(kPointer context, GoSensor sensor, GoDataSet dataset);
  static kStatus OnSystem(kPointer context, GoSystem system, GoDataSet data);
	virtual kStatus OnData(GoSensor sensor, GoDataSet dataset);
  virtual kStatus OnSystem(GoSystem system, GoDataSet dataset);

public:

  void RunForever();
	void Start();
	void Stop();
	GocatorReader(moodycamel::BlockingReaderWriterQueue<cads::msg>&);
  GocatorReader(moodycamel::BlockingReaderWriterQueue<cads::msg>&, bool);
	virtual ~GocatorReader();
};




}


