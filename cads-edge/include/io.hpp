#pragma once

#include <concepts>
#include <utility>

namespace cads
{
  template<class T, class... Args> concept IO = requires(T io, Args&&... args){
    {io.enqueue(std::forward<Args>(args)...)} -> std::same_as<bool>;
    {io.wait_dequeue(std::forward<Args&>(args)...)} -> std::same_as<void>;
  };
}
