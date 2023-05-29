#pragma once

#include <readerwriterqueue.h>
#include <msg.h>

namespace cads
{
  void window_processing_thread(double x_resolution, double y_resolution, int width_n, moodycamel::BlockingReaderWriterQueue<msg> &profile_fifo, moodycamel::BlockingReaderWriterQueue<msg> &next_fifo);
  void bypass_fiducial_detection_thread(moodycamel::BlockingReaderWriterQueue<msg> &profile_fifo, moodycamel::BlockingReaderWriterQueue<msg> &next_fifo);
  void splice_detection_thread(double x_resolution, double y_resolution, int width_n, moodycamel::BlockingReaderWriterQueue<msg> &profile_fifo);
}