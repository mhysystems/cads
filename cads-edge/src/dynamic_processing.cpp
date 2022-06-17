#include <lua.hpp>

#include <dynamic_processing.h>

namespace cads
{

  static int array_index(lua_State *L)
  {
    double *p = (double *)lua_topointer(L, 1);
    int index = lua_tointeger(L, 2);
    lua_pushnumber(L, *p);
    return 1;
  }

  static int array_newindex(lua_State *L)
  {
    double *p = (double *)lua_topointer(L, 1);
    int index = lua_tointeger(L, 2);
    auto value = lua_tonumber(L, 3);
    *p = value;
    return 0;
  }

  void inject_global_array(lua_State *L, void *p)
  {
    luaL_Reg fields[]{
        {"__index", array_index},
        {"__newindex", array_newindex}};

    luaL_newmetatable(L, "cads.window");
    luaL_setfuncs(L, fields, 0);
    lua_pushlightuserdata(L, p);
    luaL_setmetatable(L, "cads.window");
    lua_setglobal(L, "win");
  }

  coro<int, profile> lua_processing_coro()
  {
    float T = 9.0;
    lua_State *L = luaL_newstate();
    luaL_openlibs(L);
    inject_global_array(L, &T);
    const char *s = "function demo() return win[1] end";

    if (luaL_dostring(L, s) == LUA_OK)
    {

      lua_Number result = 0.0;
      lua_getglobal(L, "demo");
      if (lua_isfunction(L, -1))
      {
        lua_pcall(L, 0, 1, 0);
        result = lua_tonumber(L, -1);
      }

      co_yield (int) result;
    }

    profile p;

    // while (true)
    {

      //  p = co_yield 2;
    }

    lua_close(L);
  }
}