#pragma once

#include <string>
#include <tuple>

#include <lua.hpp>
#include <blockingconcurrentqueue.h>

namespace cads
{
  using Lua = std::unique_ptr<lua_State, decltype(&lua_close)>;
  int main_script(std::string);
  std::tuple<Lua,bool> run_lua_code(std::string);
  std::tuple<Lua,bool> run_lua_config(std::string);
  void push_externalmsg(lua_State*, moodycamel::BlockingConcurrentQueue<std::tuple<std::string, std::string, std::string>> *queue);
  
}