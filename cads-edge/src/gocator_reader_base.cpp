#include <algorithm>
#include <signal.h> 

#include <gocator_reader_base.h>



namespace cads
{
  std::atomic<bool> GocatorReaderBase::terminate = false;

  void GocatorReaderBase::sigint_handler(int s) {
    terminate = true;
  }

  GocatorReaderBase::GocatorReaderBase(moodycamel::BlockingReaderWriterQueue<msg> &gocatorFifo) : m_gocatorFifo(gocatorFifo) 
  {
    struct sigaction sigIntHandler;

    sigIntHandler.sa_handler = sigint_handler;
    sigemptyset(&sigIntHandler.sa_mask);
    sigIntHandler.sa_flags = 0;

    sigaction(SIGINT, &sigIntHandler, NULL);

  }

  z_type GocatorReaderBase::k16sToFloat(k16s *z_start, k16s *z_end, double z_resolution, double z_offset)
  {
    z_type rtn;
    std::transform(z_start, z_end, std::back_inserter(rtn), [=](k16s e)
                   { return e != k16S_NULL ? e * z_resolution + z_offset : std::numeric_limits<z_element>::quiet_NaN(); });

    return rtn;
  }

}
