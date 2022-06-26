#include <vector>
#include <cstring>

#include <lua.hpp>
#include <spdlog/spdlog.h>

#include <dynamic_processing.h>
#include <constants.h>
#include <msg.h>

namespace cads
{

  static int array_index(lua_State *L)
  {
    auto *p = (z_element *)lua_topointer(L, 1);
    int index = lua_tointeger(L, 2);
    lua_pushnumber(L, *(p + index - 1));
    return 1;
  }

  static int array_newindex(lua_State *L)
  {
    auto *p = (z_element *)lua_topointer(L, 1);
    int index = lua_tointeger(L, 2);
    auto value = lua_tonumber(L, 3);
    *(p + index - 1) = (z_element)value;
    return 0;
  }

  void inject_global_array(lua_State *L, void *p)
  {
    luaL_Reg fields[]{
        {"__index", array_index},
        {"__newindex", array_newindex},
        {NULL,NULL}};

    auto e = luaL_newmetatable(L, "cads.window");
    luaL_setfuncs(L, fields, 0);
    lua_pushlightuserdata(L, p);
    luaL_setmetatable(L, "cads.window");
    lua_setglobal(L, "win");
  }

  double eval_lua_process(lua_State *L, int width, int height)
  {

    lua_getglobal(L, "process");
    lua_pushnumber(L, width);
    lua_pushnumber(L, height);
    lua_pcall(L, 2, 1, 0);
    auto rtn = lua_tonumber(L, -1);
    lua_remove(L,-1);

    // Can remove if confident lua_stack is not growing
    auto dbg = lua_gettop(L);
    if( dbg != 1) {
      std::runtime_error("eval_lua_process:Expected lua stack size of one");
    }

    return rtn;
  }

  coro<double, msg> lua_processing_coro(int width)
  {
    auto height = global_config["lua_window_height"].get<int>();
    auto window = std::vector<z_element>(width * height, 33.0/*NaN<z_element>::value*/);

    lua_State *L = luaL_newstate();
    luaL_openlibs(L);
    inject_global_array(L, window.data());
    
    auto lua_fn = global_config["lua_fn"].get<std::string>();

   if (luaL_dostring(L, lua_fn.c_str()) != LUA_OK)
    {
      std::throw_with_nested(std::runtime_error("lua_processing_coro:Creating Lua functin failed"));
    }

    double result = 0.0;
    int64_t cnt = 0;
    cads::msg m;
    bool terminate = false;

    do
    {
      std::tie(m,terminate) = co_yield result;

      if(terminate) break;
      
      switch (get<0>(m))
      {
      case msgid::finished:
        break;
      case msgid::scan:
      {
        auto p = get<profile>(get<1>(m));
        memmove(window.data() + width, window.data(), width*(height-1)*sizeof(z_element)); // shift 2d array by one row
        memcpy(window.data(), p.z.data(), size_t(width)*sizeof(z_element));
        
        result = 0.0;
        if(cnt++ % (int64_t)height == 0) {
          result = eval_lua_process(L,width,height);
        }
        
        break;
      }
      default:
        std::throw_with_nested(std::runtime_error("lua_processing_coro:Unknown msg id"));
      }

    } while (get<0>(m) != msgid::finished || !terminate);

    lua_close(L);
    spdlog::get("cads")->info("lua_processing_coro finished");
  }


}