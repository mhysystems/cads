#pragma once

#include <string>
#include <tuple>
#include <lua.hpp>

namespace cads
{
  using Lua = std::unique_ptr<lua_State, decltype(&lua_close)>;
  int main_script(std::string);
  std::tuple<Lua,bool> run_lua_code(std::string);
  std::tuple<Lua,bool> run_lua_config(std::string);
  
}