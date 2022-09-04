#pragma once

#include <readerwriterqueue.h>
#include <msg.h>

namespace cads
{
   void dynamic_processing_thread(moodycamel::BlockingReaderWriterQueue<msg> &profile_fifo, moodycamel::BlockingReaderWriterQueue<msg> &next_fifo, int width);
}