#include <gocator_reader_base.h>
#include <algorithm>

namespace cads
{
  GocatorReaderBase::GocatorReaderBase(moodycamel::BlockingReaderWriterQueue<msg> &gocatorFifo) : m_gocatorFifo(gocatorFifo) {}

  z_type GocatorReaderBase::k16sToFloat(k16s *z_start, k16s *z_end, double z_resolution, double z_offset)
  {
    z_type rtn;
    std::transform(z_start, z_end, std::back_inserter(rtn), [=](k16s e)
                   { return e != k16S_NULL ? e * z_resolution + z_offset : cads::NaN<z_element>::value; });

    return rtn;
  }

}
