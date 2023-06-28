#pragma once

#include <tuple>
#include <string>
#include <variant>
#include <thread>
#include <functional>

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wuseless-cast"

#include <date/date.h>
#include <date/tz.h>

#pragma GCC diagnostic pop

#include <blockingconcurrentqueue.h>


namespace cads
{ 
  

  class Measure {
    
    using MeasureMsg = std::tuple<std::string,int,date::utc_clock::time_point,std::variant<double,std::string,std::function<double()>,std::function<std::string()>, std::tuple<double,double>>>;

    friend void measurement_thread(moodycamel::BlockingConcurrentQueue<Measure::MeasureMsg>&, std::string, bool&);
    friend void swap(Measure&, Measure&);
    
    Measure(const Measure &) = delete;
    Measure& operator= (const Measure&) = delete;
    
    public:
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
    
    bool terminate = false;
    moodycamel::BlockingConcurrentQueue<MeasureMsg> fifo;
    std::jthread thread;
     
  };


}