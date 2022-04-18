#pragma once

#include <readerwriterqueue.h>

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
	moodycamel::BlockingReaderWriterQueue<char>& m_gocatorFifo;

public:

  virtual void RunForever() = 0;
	virtual void Start() = 0;
	virtual void Stop() = 0;
	GocatorReaderBase(moodycamel::BlockingReaderWriterQueue<char>&);

};


}

