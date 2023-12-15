#pragma once

#include <tuple>

#include <io.hpp>
#include <fiducial.h>
#include <constants.h>

namespace cads
{
  struct FiducialOriginDetection {
    using value_type = std::tuple<double,double>;
    value_type belt_length;
    double cross_correlation_threshold;
    bool   dump_match;
    Fiducial fiducial;
    double length;
    double avg_y_res;
    double width_n;
  };
  
  struct AnomalyDetection {
    size_t WindowSize;
    size_t BeltPartitionSize;
    size_t BeltSize;
    size_t MinPosition;
    size_t MaxPosition;
    std::string ConveyorName;
  };

  void fiducial_origin_thread(FiducialOriginDetection config,cads::Io<cads::msg> &profile_fifo, cads::Io<cads::msg> &next_fifo);
  void splice_detection_thread(cads::AnomalyDetection anomaly, cads::Io<cads::msg> &profile_fifo, cads::Io<cads::msg> &next_fifo);
  void loop_beltlength_thread(double length, cads::Io<cads::msg> &profile_fifo, cads::Io<cads::msg> &next_fifo);
}