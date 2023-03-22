#include <thread>

#include <lua.hpp>
#include <readerwriterqueue.h>

#include <msg.h>
#include <origin_detection_thread.h>
#include <save_send_thread.h>

namespace
{
  int BlockingReaderWriterQueue_gc(lua_State *L)
  {
    auto q = static_cast<moodycamel::BlockingReaderWriterQueue<cads::msg> *>(lua_touserdata(L, 1));
    q->~BlockingReaderWriterQueue<cads::msg>();
    return 0;
  }

  int thread_gc(lua_State *L)
  {
    auto t = static_cast<std::thread *>(lua_touserdata(L, 1));
    t->~thread();
    return 0;
  }

  int luamain(lua_State *L)
  {
    int array_length = lua_rawlen(L, 1);

    for (int i = 1; i <= array_length; i++) {
        lua_rawgeti(L, 1, i);
        auto v = lua_touserdata(L,-1);
        auto t = static_cast<std::thread*>(v);
        t->join();
        lua_pop(L, 1);
    }

    lua_pushnumber(L, 3);
    return 1;
  }

  int BlockingReaderWriterQueue(lua_State *L)
  {
    new (lua_newuserdata(L, sizeof(moodycamel::BlockingReaderWriterQueue<cads::msg>))) moodycamel::BlockingReaderWriterQueue<cads::msg>();

    luaL_getmetatable(L, "BlockingReaderWriterQueue");
    lua_setmetatable(L, -2);

    return 1;
  }

  int mk_thread(lua_State *L, std::function<void(moodycamel::BlockingReaderWriterQueue<cads::msg>&)> fn)
  {
    auto q = static_cast<moodycamel::BlockingReaderWriterQueue<cads::msg>*>(lua_touserdata(L,-1));
    new (lua_newuserdata(L,sizeof(std::thread))) std::thread(fn,ref(*q));
    lua_setmetatable(L, -2);
    return 1;
  }

  int mk_thread2(lua_State *L, std::function<void(moodycamel::BlockingReaderWriterQueue<cads::msg>&,moodycamel::BlockingReaderWriterQueue<cads::msg>&)> fn)
  {
    auto q1 = static_cast<moodycamel::BlockingReaderWriterQueue<cads::msg>*>(lua_touserdata(L,-1));
    auto q0 = static_cast<moodycamel::BlockingReaderWriterQueue<cads::msg>*>(lua_touserdata(L,-2));
    new (lua_newuserdata(L,sizeof(std::thread))) std::thread(fn,ref(*q0),ref(*q1));
    lua_setmetatable(L, -2);
    return 1;
  }
  
  int bypass_fiducial_detection(lua_State *L)
  {
    return mk_thread2(L,cads::bypass_fiducial_detection_thread);
  }

  int save_send(lua_State *L) {
    return mk_thread2(L,cads::save_send_thread); // Need to pass in z_offset, z_resolutions
  }

}

using lua_unique = std::unique_ptr<lua_State, decltype(&lua_close)>;
namespace cads
{
  namespace lua
  {
    
    lua_unique init()
    {
      lua_State *L = luaL_newstate();
      luaL_openlibs(L);
      
      luaL_newmetatable(L, "thread");
      lua_pushcfunction(L, thread_gc);
      lua_setfield(L, -2, "__gc");

      luaL_newmetatable(L, "BlockingReaderWriterQueue");
      lua_pushcfunction(L, BlockingReaderWriterQueue_gc);
      lua_setfield(L, -2, "__gc");

      lua_pushcfunction(L, BlockingReaderWriterQueue);
	    lua_setglobal(L,"BlockingReaderWriterQueue");

      lua_pushcfunction(L, bypass_fiducial_detection);
	    lua_setglobal(L,"bypass_fiducial_detection");

      lua_pushcfunction(L, save_send);
	    lua_setglobal(L,"save_send");
      
      lua_pushcfunction(L, luamain);
      lua_setglobal(L,"luamain");



      
      return lua_unique(L, &lua_close);
    }
    

  }
}