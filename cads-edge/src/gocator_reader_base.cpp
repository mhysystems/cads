#include <gocator_reader_base.h>

namespace cads
{
  GocatorReaderBase::GocatorReaderBase(moodycamel::BlockingReaderWriterQueue<char>& gocatorFifo) : m_gocatorFifo(gocatorFifo){}
}

