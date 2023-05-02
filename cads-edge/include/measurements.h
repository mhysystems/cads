#pragma once

#include <tuple>
#include <string>
#include <variant>
#include <thread>

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wuseless-cast"

#include <date/date.h>
#include <date/tz.h>

#pragma GCC diagnostic pop

#include <readerwriterqueue.h>


namespace cads
{ 
  

  class Measure {
    
    enum MeasureType{mDouble,mString};
    using MeasureMsg = std::tuple<std::string,std::variant<double,std::string>,date::utc_clock::time_point>;

    friend void measurement_thread(moodycamel::BlockingReaderWriterQueue<Measure::MeasureMsg>&, bool&);

    Measure(const Measure &) = delete;
    Measure& operator= (const Measure&) = delete;
    
    public:
    Measure() = default;  
    ~Measure();
    void init();
    void send(std::string, double);
    void send(std::string, std::string);

    protected:
    
    bool terminate = false;
    moodycamel::BlockingReaderWriterQueue<MeasureMsg> fifo;
    std::jthread thread;
     
  };


}