#include <coroutine>
#include <tuple>

namespace cads
{
  template <typename T, typename R = int>
  struct coro
  {
    struct promise_type;
    using handle_type = std::coroutine_handle<promise_type>;

    struct pass_value
    {
      R value_out;
      pass_value() = default;
      pass_value(R &a) : value_out(a) {}
      constexpr bool await_ready() const noexcept { return false; }

      constexpr void await_suspend(handle_type) const noexcept {}

      R await_resume() const noexcept
      {
        return value_out;
      }
    };

    struct promise_type
    {
      T value_in;
      R value_out;
      std::exception_ptr exception_;

      coro get_return_object()
      {
        return coro(handle_type::from_promise(*this));
      }

      std::suspend_always initial_suspend() noexcept { return {}; }
      std::suspend_always final_suspend() noexcept { return {}; }
      void unhandled_exception() { exception_ = std::current_exception(); }

      pass_value yield_value(T from)
      {
        value_in = from;
        return {value_out};
      }

      void return_value(T from)
      {
        value_in = from;
      }

    };

    handle_type coro_hnd;

    coro(handle_type h) : coro_hnd(h) {}
    ~coro() { coro_hnd.destroy(); }

    std::tuple<bool, T> operator()(R a)
    {
      coro_hnd.promise().value_out = a;
      if (!coro_hnd.done())
        coro_hnd.resume();
      return {coro_hnd.done(), coro_hnd.promise().value_in};
    }
  };
} // namespace cads
