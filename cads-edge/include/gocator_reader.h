#ifndef CADS_RECORDER_CSV_RECORDER_H
#define CADS_RECORDER_CSV_RECORDER_H

#include <GoSdk/GoSensor.h>
#include <GoSdk/GoSystem.h>
#include <kApi/kApiDef.h>


#include <readerwriterqueue.h>
#include <condition_variable>
#include <atomic>
#include <mutex>

namespace cads
{

class GocatorReader
{

	GocatorReader() = delete;
	GocatorReader(const GocatorReader&) = delete;
	GocatorReader& operator=(const GocatorReader&) = delete;
	GocatorReader(GocatorReader&&) = delete;
	GocatorReader& operator=(GocatorReader&&) = delete;

protected:
	std::atomic<bool> m_loop;
	double m_yResolution;
	moodycamel::BlockingReaderWriterQueue<char>& m_gocatorFifo;
	
	GoSystem m_system;
	kAssembly m_assembly;
	GoSensor m_sensor;

	std::condition_variable m_condition;
  std::mutex m_mutex;

	static kStatus OnData(kPointer context, GoSensor sensor, GoDataSet dataset);
	virtual kStatus OnData(GoSensor sensor, GoDataSet dataset);

public:

  void RunForever();
	void Start();
	void Stop();
	GocatorReader(moodycamel::BlockingReaderWriterQueue<char>&);
	~GocatorReader();
};




}

#endif
