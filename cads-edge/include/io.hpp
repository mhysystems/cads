#pragma once

#include <concepts>
#include <utility>
#include <chrono>

#include <msg.h> 

namespace cads
{
  template<class T, class... Args> concept IO = requires(T io, Args&&... args){
    {io.enqueue(std::forward<Args>(args)...)} -> std::same_as<bool>;
    {io.wait_dequeue(std::forward<Args&>(args)...)} -> std::same_as<void>;
    //{io.wait_dequeue_timed(std::forward<Args&>(args)...)} -> std::same_as<bool>;
    {io.size_approx()} -> std::same_as<size_t>;
  };

  template<class T> struct Io {
    Io() = default;
    virtual ~Io() = default;
    virtual bool enqueue(T) = 0;
    virtual void wait_dequeue(T&) = 0;
    virtual bool wait_dequeue_timed(T& x, std::chrono::seconds s) = 0;
    virtual size_t size_approx() = 0;
    
    private:
    Io(const Io&) = delete;
    Io& operator=(const Io&) = delete;
};

  template<class B, IO<B> T> struct Adapt : public Io<B>
  {
    Adapt(T&& a) : m(std::move(a)) {} 
    
    virtual bool enqueue(B x) {
      return m.enqueue(x);
    }

    virtual void wait_dequeue(B& x) {
      return m.wait_dequeue(x);
    }

    virtual size_t size_approx() {
      return m.size_approx();
    }

    virtual bool wait_dequeue_timed(B& x, std::chrono::seconds s) {
      return m.wait_dequeue_timed(x,s);
    }

    private:
    Adapt(const Adapt&) = delete;
    Adapt() = delete;
    Adapt& operator=(const Adapt&) = delete;
    T m;
  };

  template<class T,class R> struct AdaptFn : public Io<T>
  {
    AdaptFn(std::function<R(T)>&& a) : m(std::move(a)) {} 
    
    virtual R enqueue(T x) {
      return m(x);
    }

    virtual void wait_dequeue(T&) {
    }

    virtual size_t size_approx() {
      return 0;
    }

    virtual bool wait_dequeue_timed(T&,std::chrono::seconds) {
      return false;
    }

    private:
    AdaptFn(const AdaptFn&) = delete;
    AdaptFn() = delete;
    AdaptFn& operator=(const AdaptFn&) = delete;
    std::function<bool(T)> m;
  };

}
