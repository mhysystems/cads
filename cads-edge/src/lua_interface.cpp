#include <thread>
#include <filesystem>

#include <lua.hpp>
#include <readerwriterqueue.h>

#include <msg.h>
#include <origin_detection_thread.h>
#include <save_send_thread.h>
#include <cads.h>
#include <belt.h>
#include <io.hpp>

namespace
{
  int Io_gc(lua_State *L)
  {
    auto q = static_cast<cads::Io *>(lua_touserdata(L, 1));
    q->~Io();
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
      auto p = new (lua_newuserdata(L, sizeof(cads::Adapt<moodycamel::BlockingReaderWriterQueue<cads::msg>>))) cads::Adapt<moodycamel::BlockingReaderWriterQueue<cads::msg>> (moodycamel::BlockingReaderWriterQueue<cads::msg>());

      lua_createtable(L, 0, 1); 
      lua_pushcfunction(L, Io_gc);
      lua_setfield(L, -2, "__gc");
      lua_setmetatable(L, -2);

    return 1;
  }

  int mk_thread(lua_State *L, std::function<void(cads::Io&)> fn)
  {
    auto q = static_cast<cads::Io*>(lua_touserdata(L,-1));
    new (lua_newuserdata(L,sizeof(std::thread))) std::thread(fn,std::ref(*q));
    lua_createtable(L, 0, 1); 
    lua_pushcfunction(L, thread_gc);
    lua_setfield(L, -2, "__gc");
    lua_setmetatable(L, -2);
    return 1;
  }

  int mk_thread2(lua_State *L, std::function<void(cads::Io&,cads::Io&)> fn)
  {
    auto in = static_cast<cads::Io*>(lua_touserdata(L,1));
    auto out = static_cast<cads::Io*>(lua_touserdata(L,2));
    out->enqueue(cads::msg());
    new (lua_newuserdata(L,sizeof(std::thread))) std::thread(fn,std::ref(*in),std::ref(*out));
    lua_createtable(L, 0, 1); 
    lua_pushcfunction(L, thread_gc);
    lua_setfield(L, -2, "__gc");
    lua_setmetatable(L, -2);
    return 1;
  }

  int gocator_start(lua_State *L) 
  {
    auto gocator = static_cast<std::unique_ptr<cads::GocatorReaderBase>*>(lua_touserdata(L,-1));
    (*gocator)->Start();

    return 0;
  }

  int gocator_stop(lua_State *L) 
  {
    auto gocator = static_cast<std::unique_ptr<cads::GocatorReaderBase>*>(lua_touserdata(L,-1));
    (*gocator)->Stop();

    return 0;
  }

  int gocator_gc(lua_State *L) 
  {
    auto gocator = static_cast<std::unique_ptr<cads::GocatorReaderBase>*>(lua_touserdata(L,-1));
    gocator->~unique_ptr<cads::GocatorReaderBase>();

    return 0;
  }

  int mk_gocator(lua_State *L)
  {
    auto q = static_cast<cads::Io*>(lua_touserdata(L, 1));
    auto trim = lua_toboolean(L,2);
    auto use_encoder = lua_toboolean(L,3);

    auto p = new (lua_newuserdata(L, sizeof(decltype(cads::mk_gocator(*q,trim,use_encoder))))) decltype(cads::mk_gocator(*q,trim,use_encoder));
    *p = cads::mk_gocator(*q,trim,use_encoder);
    
    
    lua_createtable(L, 0, 1); 
    lua_pushcfunction(L, gocator_gc);
    lua_setfield(L, -2, "__gc");
    lua_createtable(L,0,2);
    lua_pushcfunction(L, gocator_start);
    lua_setfield(L, -2, "Start");
    lua_pushcfunction(L, gocator_stop);
    lua_setfield(L, -2, "Stop");
    lua_setfield(L, -2, "__index");
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

  int process_profile(lua_State *L) {
    return mk_thread2(L,cads::process_lua);
  }

  int encoder_distance_estimation(lua_State *L) {
    auto next = static_cast<cads::Io*>(lua_touserdata(L,1));
    double stride = lua_tonumber(L,2);
    auto p = new (lua_newuserdata(L, sizeof(cads::Adapt<decltype(cads::encoder_distance_estimation(std::ref(*next),stride))>))) cads::Adapt<decltype(cads::encoder_distance_estimation(std::ref(*next),stride))>(cads::encoder_distance_estimation(std::ref(*next),stride));

    lua_createtable(L, 0, 1); 
    lua_pushcfunction(L, Io_gc);
    lua_setfield(L, -2, "__gc");
    lua_setmetatable(L, -2);
    return 1;

  }

}

using Lua = std::unique_ptr<lua_State, decltype(&lua_close)>;
namespace cads
{
  namespace lua
  {
    
    Lua init(Lua UL)
    {
      auto L = UL.get();

      lua_pushcfunction(L, BlockingReaderWriterQueue);
	    lua_setglobal(L,"BlockingReaderWriterQueue");

      lua_pushcfunction(L, bypass_fiducial_detection);
	    lua_setglobal(L,"bypass_fiducial_detection");

      lua_pushcfunction(L, save_send);
	    lua_setglobal(L,"save_send");
      
      lua_pushcfunction(L, luamain);
      lua_setglobal(L,"luamain");

      lua_pushcfunction(L, ::mk_gocator);
      lua_setglobal(L,"mk_gocator");

      lua_pushcfunction(L, ::process_profile);
      lua_setglobal(L,"process_profile");
      
      lua_pushcfunction(L, ::encoder_distance_estimation);
      lua_setglobal(L,"encoder_distance_estimation");
      
      
      return UL;
    }

  }

  int main_script(std::string f) {

      namespace fs = std::filesystem;

      fs::path luafile{f};
      luafile.replace_extension("lua");

      auto L = Lua{luaL_newstate(),lua_close};
      luaL_openlibs( L.get() );
      
      L = lua::init(std::move(L));

      auto lua_status = luaL_dofile(L.get(),luafile.string().c_str());
      
      if(lua_status != LUA_OK) {
        return -1;
      }

      return 0;
    }
}