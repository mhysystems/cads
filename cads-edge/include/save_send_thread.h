#pragma once

#include <readerwriterqueue.h>
#include <msg.h>

namespace cads
{
  void save_send_thread(moodycamel::BlockingReaderWriterQueue<msg> &profile_fifo,double z_offset, double z_resolution);
}