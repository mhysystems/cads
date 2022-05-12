#include <gocator_reader_base.h>

namespace cads
{
  GocatorReaderBase::GocatorReaderBase(moodycamel::BlockingReaderWriterQueue<profile>& gocatorFifo) : m_gocatorFifo(gocatorFifo){}
  
  std::tuple<double,double,double,double,double> GocatorReaderBase::get_gocator_constants() {
    {
      std::unique_lock<std::mutex> lock(m_mutex);
      m_condition.wait(lock);
    }

    return {m_yResolution,m_xResolution,m_zResolution,m_zOffset,m_encoder_resolution};
  }


}

