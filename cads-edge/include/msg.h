#pragma once 

#include <variant>
#include <tuple>
#include <string>

#include <profile.h>
#include <readerwriterqueue.h>
#include <measurements.h>

namespace cads
{ 
  using PulleyRevolutionScan = std::tuple<bool, double, cads::profile>;
  enum msgid{gocator_properties,scan,finished,begin_sequence,end_sequence,complete_belt,pulley_revolution_scan,stopped,nothing,select,realtime_metric,measure};
  struct GocatorProperties{double xResolution; double zResolution; double zOffset;};
  
  struct RealtimeMetric {
    std::string subject;
    std::string header;
    std::string data;
  };

  using scan_t = struct {cads::profile value;};
  using begin_sequence_t = struct {};
  using end_sequence_t = struct {};
  
  struct Select {
    moodycamel::BlockingReaderWriterQueue<std::deque<std::tuple<int, cads::z_type>>>* fifo;
    size_t begin;
    size_t size;
  };

  struct CompleteBelt {size_t start_value; size_t length;};
  using msg = std::tuple<msgid,std::variant<cads::GocatorProperties,cads::profile,std::string,long,double,cads::PulleyRevolutionScan,cads::CompleteBelt, Select, RealtimeMetric, Measure::MeasureMsg>>;

  using Timeout = struct Timeout_s{};
  using Start = struct Start_s {std::string lua_code;};
  using Stop = struct Stop_s{};
  using remote_msg = std::variant<Start,Stop,Timeout>;
}