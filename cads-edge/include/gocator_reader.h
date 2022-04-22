#ifndef CADS_RECORDER_CSV_RECORDER_H
#define CADS_RECORDER_CSV_RECORDER_H

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

	static kStatus OnData(kPointer context, GoSensor sensor, GoDataSet dataset);
	virtual kStatus OnData(GoSensor sensor, GoDataSet dataset);

public:

  void RunForever();
	void Start();
	void Stop();
	GocatorReader(moodycamel::BlockingReaderWriterQueue<cads::profile>&);
	~GocatorReader();
};




}

#endif
