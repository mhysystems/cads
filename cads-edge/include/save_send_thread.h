#pragma once

#include <readerwriterqueue.h>
#include <msg.h>

namespace cads
{
  void save_send_thread(moodycamel::BlockingReaderWriterQueue<msg> &profile_fifo, moodycamel::BlockingReaderWriterQueue<msg> &next);
}