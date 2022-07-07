#pragma once 

#include <coroutine>
#include <tuple>
#include <exception>
#include <type_traits>

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
      TC& to_coro;
      bool& terminate;
      awaitable() = default;
      awaitable(TC& a, bool& t) : to_coro(a),terminate(t) {}
      
      constexpr bool await_ready() const noexcept { return false; }

      // Called when coroutine suspends
      // Can return the next coroutine if this class has a reference to one 
      constexpr void await_suspend(handle_type) const noexcept {}

      // Called when coroutine is resumed
      std::tuple<TC,bool> await_resume() const noexcept
      {
        return {to_coro,terminate};
      }
    };

    struct promise_type
    {
      FC from_coro;
      TC to_coro;
      bool terminate_coro = false;

      std::exception_ptr exception_;

      // Called when coroutine give back control
      coro get_return_object()
      {
        return coro(handle_type::from_promise(*this));
      }

      // Inserted before the coroutine body is called.
      std::conditional<dir == 0,std::suspend_always,std::suspend_never>::type initial_suspend() noexcept { return {}; }
      
      // Inserted after the coroutine body is finished. Using suspend_never cause memleak detector to complain
      std::suspend_always final_suspend() noexcept { return {}; } 
      
      // try catch inserted around coroutine body, called if exception is not caught in coroutine.
      void unhandled_exception() { exception_ = std::current_exception(); }

      // Called by co_yield
      awaitable yield_value(FC from)
      {
        from_coro = from;
        return {to_coro,terminate_coro};
      }

      void return_value(FC from)
      {
        from_coro = from;
      }

    };

    handle_type coro_hnd;

    coro(handle_type h) : coro_hnd(h) {}
    coro(const coro&) = delete;
    coro(coro&& c) = delete;
    ~coro() { 
      if (!coro_hnd.done()) {
        terminate();
      }
      coro_hnd.destroy(); 
    }

    std::tuple<bool, FC> resume(TC a)
    {
      coro_hnd.promise().to_coro = a;
      
      if (!coro_hnd.done())
        coro_hnd.resume(); // Eventually calls await_resume
      
      return {coro_hnd.done(), coro_hnd.promise().from_coro};
    }

    void terminate()
    {
      coro_hnd.promise().terminate_coro = true;
      if (!coro_hnd.done())
        coro_hnd.resume(); // Eventually calls await_resume
    }
  };
} // namespace cads
