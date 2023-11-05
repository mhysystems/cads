#pragma once 

#include <coroutine>
#include <tuple>
#include <exception>
#include <type_traits>
#include <chrono>

#include <msg.h>

namespace cads
{
  template <typename FC, typename TC = int, int dir = 0>
  struct coro
  {
    struct promise_type;
    using handle_type = std::coroutine_handle<promise_type>;

    // Class passed to co_wait or co_yield ie. co_await promise.yield_value(expr)
    struct awaitable
    {
      const promise_type &promise;
      
      awaitable(const promise_type &p) : promise(p) {}
      
      // To keep track of what is used
      awaitable() = delete;
      awaitable(promise_type &&p) = delete;
      awaitable& operator=(const awaitable&) = delete;
      awaitable& operator=(awaitable&&) = delete;
     
      constexpr bool await_ready() const noexcept { return false; }

      // Called when coroutine suspends
      // Can return the next coroutine if this class has a reference to one 
      constexpr void await_suspend(handle_type) const noexcept {}

      // Called when coroutine is resumed
      std::tuple<TC,bool> await_resume() const noexcept
      {
        return {promise.to_coro,promise.terminate_coro};
      }
    };

    struct promise_type
    {
      FC from_coro;
      TC to_coro;
      bool terminate_coro = false;

      std::exception_ptr exception_;

      // Need this constructor
      promise_type() = default;

      // To keep track of what is used
      promise_type(const promise_type&) = delete;
      promise_type(promise_type&&) = delete;
      promise_type& operator=(const promise_type&) = delete;
      promise_type& operator=(promise_type&&) = delete;
      
      // Called when coroutine is constructed
      coro get_return_object()
      {
        return coro(handle_type::from_promise(*this));
      }

      // Inserted before the coroutine body is called.
      // dir 0 means the first co_yield will yield control on first resume as coroutine starts suspended
      // dir 1 means the first co_yeild will yield control when the coroutine is constructed.
      std::conditional<dir == 0,std::suspend_always,std::suspend_never>::type initial_suspend() noexcept { return {}; }
      
      // Inserted after the coroutine body is finished. Using suspend_never cause memleak detector to complain
      std::suspend_always final_suspend() noexcept { return {}; } 
      
      // try catch inserted around coroutine body, called if exception is not caught in coroutine.
      void unhandled_exception() { exception_ = std::current_exception(); }

      // Called by co_yield
      awaitable yield_value(FC from)
      {
        from_coro = from;
        return {*this};
      }

      void return_value(FC from)
      {
        from_coro = from;
      }

    };

    handle_type coro_hnd;
    bool valid_handle = false;


    friend void swap(coro& first, coro& second)
    {
      std::swap(first.coro_hnd, second.coro_hnd);
      std::swap(first.valid_handle, second.valid_handle);
    }
    
    coro(handle_type h) : coro_hnd(h), valid_handle(true){}
    coro& operator=(coro&& rhs) noexcept
    {
      swap(*this,rhs);
      return *this;
    }
    
    
    // To keep track of what is used
    coro() = delete;
    coro(const coro&) = delete;    
    coro& operator=(const coro&) = delete;
    
    coro(coro&& c) {
      swap(*this,c);
    };

    
    
    ~coro() { 
      
      if(valid_handle && coro_hnd) {
        if (!coro_hnd.done()) {
          terminate();
        }
        coro_hnd.destroy(); 
      }

    }

    std::tuple<bool, FC> resume(TC a)
    {
      coro_hnd.promise().to_coro = a;
      
      if (!coro_hnd.done())
        coro_hnd.resume(); // Eventually calls await_resume
      
      return {coro_hnd.done(), coro_hnd.promise().from_coro};
    }

    bool enqueue(TC a) {
      return std::get<0>(resume(a));
    }

    void wait_dequeue(FC& fc) {
      TC tc;
      bool i;
      std::tie(i,fc) = resume(tc);
    }

    bool wait_dequeue_timed(FC& x, [[maybe_unused]]std::chrono::seconds s) {
      wait_dequeue(x);
      return true;
    }

    size_t size_approx() {
      return 1;
    }

    bool is_done()
    {
      return coro_hnd.done();
    }

    void terminate()
    {
      coro_hnd.promise().terminate_coro = true;
      if (!coro_hnd.done())
        coro_hnd.resume(); // Eventually calls await_resume
    }

  };
} // namespace cads
