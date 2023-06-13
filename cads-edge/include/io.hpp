#pragma once

#include <concepts>
#include <utility>

#include <msg.h> 

namespace cads
{
  template<class T, class... Args> concept IO = requires(T io, Args&&... args){
    {io.enqueue(std::forward<Args>(args)...)} -> std::same_as<bool>;
    {io.wait_dequeue(std::forward<Args&>(args)...)} -> std::same_as<void>;
    {io.size_approx()} -> std::same_as<size_t>;
  };

  struct Io {
    Io() = default;
    virtual ~Io() = default;
    virtual bool enqueue(msg) = 0;
    virtual void wait_dequeue(msg&) = 0;
    virtual size_t size_approx() = 0;
    
    private:
    Io(const Io&) = delete;
    Io& operator=(const Io&) = delete;
};

  template<IO<msg> T> struct Adapt : public Io
  {
    Adapt(T&& a) : m(std::move(a)) {} 
    
    virtual bool enqueue(msg x) {
      return m.enqueue(x);
    }

    virtual void wait_dequeue(msg& x) {
      return m.wait_dequeue(x);
    }

     virtual size_t size_approx() {
      return m.size_approx();
    }

    private:
    Adapt(const Adapt&) = delete;
    Adapt() = delete;
    Adapt& operator=(const Adapt&) = delete;
    T m;
  };
}
