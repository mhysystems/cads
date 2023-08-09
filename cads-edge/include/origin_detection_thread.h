#pragma once

#include <io.hpp>
#include <constants.h>

namespace cads
{
  void window_processing_thread(Conveyor conveyor,cads::Io &profile_fifo, cads::Io &next_fifo);
  void splice_detection_thread(cads::AnomalyDetection anomaly, cads::Io &profile_fifo, cads::Io &next_fifo);
  void loop_beltlength_thread(Conveyor conveyor, cads::Io &profile_fifo, cads::Io &next_fifo);
}