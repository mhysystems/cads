#pragma once

#include <io.hpp>

namespace cads
{
  void window_processing_thread(double x_resolution, double y_resolution, int width_n, cads::Io &profile_fifo, cads::Io &next_fifo);
  void bypass_fiducial_detection_thread(cads::Io &profile_fifo, cads::Io &next_fifo);
  void splice_detection_thread(double x_resolution, double y_resolution, int width_n, cads::Io &profile_fifo, cads::Io &next_fifo);
}