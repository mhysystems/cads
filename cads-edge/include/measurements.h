#pragma once

#include <tuple>
#include <string>
#include <variant>
#include <thread>
#include <functional>
#include <memory>

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wuseless-cast"

#include <date/date.h>
#include <date/tz.h>

#pragma GCC diagnostic pop

#include <blockingconcurrentqueue.h>


namespace cads
{ 
  
  struct MeasureData;

  class Measure {
    
    friend void measurement_thread(MeasureData*, std::string);
    friend void swap(Measure&, Measure&);
    
    Measure(const Measure &) = delete;
    Measure& operator= (const Measure&) = delete;
    
    public:

    using MeasureMsg = std::tuple<std::string,int,date::utc_clock::time_point,std::variant<double,std::string,std::function<double()>,std::function<std::string()>, std::tuple<double,double>>>;

    Measure(std::string);
    Measure() = delete;  
    ~Measure();
    void send(std::string, int quality, double);
    void send(std::string, int quality, std::string);
    void send(std::string, int quality, std::function<double()>);
    void send(std::string, int quality, std::function<std::string()>);
    void send(std::string, int quality, std::tuple<double,double>);

    Measure& operator= (Measure&&) noexcept;

    protected:
    
    MeasureData* data;
    std::jthread thread;
       
  };


}