#pragma once

#include <cads.h>
#include <readerwriterqueue.h>
#include <coroutine>
#include <exception>

#include <profile.h>

namespace cads
{
	template <typename T, typename R = int>
	struct generator
	{
		struct promise_type;
		using handle_type = std::coroutine_handle<promise_type>;

		struct suspend_always2
		{
			R &value_out;
			suspend_always2() = default;
			suspend_always2(R &a) : value_out(a) {}
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

			generator get_return_object()
			{
				return generator(handle_type::from_promise(*this));
			}

			std::suspend_always initial_suspend() noexcept { return {}; }
			std::suspend_always final_suspend() noexcept { return {}; }
			void unhandled_exception() { exception_ = std::current_exception(); }

			suspend_always2 yield_value(T from)
			{
				value_in = from;
				return {value_out};
			}
			void return_void() {}
		};

		handle_type coro;

		generator(handle_type h) : coro(h) {}
		~generator() { coro.destroy(); }

		bool resume()
		{
			return coro ? (coro.resume(), !coro.done()) : false;
		}

		bool resume(R a)
		{
			coro.promise().value_out = a;
			return coro ? (coro.resume(), !coro.done()) : false;
		}

		T operator()(R a)
		{
			coro.promise().value_out = a;
			return coro.promise().value_in;
		}

		T operator()()
		{
			return coro.promise().value_in;
		}
	};

  generator<gocator_profile> get_flatworld(moodycamel::BlockingReaderWriterQueue<char>& fifo);
  generator<gocator_profile> get_flatworld(moodycamel::BlockingReaderWriterQueue<cads::profile>& fifo);

}