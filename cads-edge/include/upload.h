#pragma once
#include <readerwriterqueue.h>

namespace cads
{
  void upload_scan_thread(moodycamel::BlockingReaderWriterQueue<msg> &fifo);
}