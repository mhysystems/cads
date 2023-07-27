#include <thread>
#include <filesystem>
#include <memory>

#include <readerwriterqueue.h>

#include <msg.h>
#include <origin_detection_thread.h>
#include <save_send_thread.h>
#include <cads.h>
#include <belt.h>
#include <io.hpp>
#include <dynamic_processing.h>
#include <upload.h>
#include <spdlog/spdlog.h>

#include <lua_script.h>
#include <constants.h>
#include <sqlite_gocator_reader.h>
#include <gocator_reader.h>

namespace
{
  std::optional<std::string> tostring(lua_State *L, int index)
  {
    size_t len = 0;

    if(!lua_isstring(L,index))
    {
      spdlog::get("cads")->error("{{ func = {},  msg = 'lua object not a string' }}",__func__);
      return std::nullopt;      
    }

    auto a = lua_tolstring(L,index,&len);

    return a;
  }
  
  std::optional<double> tonumber(lua_State *L, int index)
  {
    int isnum = 0;
    auto a = lua_tonumberx(L, index, &isnum);

    if(isnum == 0) {
      spdlog::get("cads")->error("{{ func = {},  msg = 'lua object not a number' }}",__func__);
      return std::nullopt;
    }

    return a;
  }

  std::optional<std::tuple<long long,long long>> topair(lua_State *L, int index) 
  {
    if(!lua_istable(L,index)) {
      spdlog::get("cads")->error("{{ func = {},  msg = 'lua object needs to be a array' }}",__func__);
      return std::nullopt;
    }
    
    int array_length = lua_rawlen(L, index);
    
    if(array_length != 2) 
    {
      spdlog::get("cads")->error("{{ func = {},  msg = 'lua object should have only 2 elements in array' }}",__func__); 
      return std::nullopt;
    }    

    int num = 0;
    if(lua_rawgeti(L, index, 1) != LUA_TNUMBER) 
    {
      spdlog::get("cads")->error("{{ func = {},  msg = 'lua array doesn't contain number at index {}' }}",__func__,1); 
      return std::nullopt;
    } 

    auto a = lua_tointegerx(L, 1, &num);  
    lua_pop(L, 1);
    
    if(num == 0)
    {
      spdlog::get("cads")->error("{{ func = {},  msg = 'lua array doesn't contain integer at index {}' }}",__func__,1); 
      return std::nullopt;
    }
    
    if(lua_rawgeti(L, index, 2) != LUA_TNUMBER) 
    {
      spdlog::get("cads")->error("{{ func = {},  msg = 'lua array doesn't contain number at index {}' }}",__func__,2); 
      return std::nullopt;
    } 

    auto b = lua_tointegerx(L, 1, &num);  
    lua_pop(L, 1);

    if(num == 0)
    {
      spdlog::get("cads")->error("{{ func = {},  msg = 'lua array doesn't contain integer at index {}' }}",__func__,2); 
      return std::nullopt;
    }

    return std::make_tuple(a,b);

  }

  std::optional<cads::SqliteGocatorConfig> tosqlitegocatorconfig(lua_State *L, int index)
  {
    if(!lua_istable(L,index)) 
    {
      spdlog::get("cads")->error("{{ func = {},  msg = 'sqlite gocator needs to be a lua table' }}",__func__);
      return std::nullopt;
    }

    lua_getfield(L, index, "Range");
    auto range_opt = topair(L,-1);
    lua_pop(L,1);

    if(!range_opt) 
    {
      spdlog::get("cads")->error("{{ func = {},  msg = 'range not a tuple pair with integers' }}",__func__);
      return std::nullopt;
    }

    lua_getfield(L, index, "Fps");
    auto fps_opt = tonumber(L,-1);
    lua_pop(L,1);

    if(!fps_opt) 
    {
      spdlog::get("cads")->error("{{ func = {},  msg = 'fps not a number' }}",__func__);
      return std::nullopt;
    }

    if(*fps_opt < 1) 
    {
      spdlog::get("cads")->error("{{ func = {},  msg = 'fps less than 1' }}",__func__);
      return std::nullopt;
    }

    lua_getfield(L, index, "Forever");
    bool forever = lua_toboolean(L,-1);
    lua_pop(L,1);

    lua_getfield(L, index, "Delay");
    auto delay_opt = tonumber(L,-1);
    lua_pop(L,1);

    if(!delay_opt) 
    {
      spdlog::get("cads")->error("{{ func = {},  msg = 'delay not a number' }}",__func__);
      return std::nullopt;
    }

    lua_getfield(L, index, "Source");
    auto source_opt = tostring(L,-1);
    lua_pop(L,1);

    if(!source_opt) 
    {
      spdlog::get("cads")->error("{{ func = {},  msg = 'source not a string' }}",__func__);
      return std::nullopt;
    }

    lua_getfield(L, index, "TypicalSpeed");
    auto typical_speed_opt = tonumber(L, -1);
    lua_pop(L, 1);
    
    if(!typical_speed_opt) 
    {
      spdlog::get("cads")->error("{{ func = {},  msg = 'source not a string' }}",__func__);
      return std::nullopt;
    }

    return cads::SqliteGocatorConfig{*range_opt,*fps_opt,forever,*delay_opt,*source_opt,*typical_speed_opt};
   
  }
  
  std::optional<cads::GocatorConfig> togocatorconfig(lua_State *L, int index)
  {
    if(!lua_istable(L,index)) 
    {
      spdlog::get("cads")->error("{{ func = {},  msg = 'sqlite gocator needs to be a lua table' }}",__func__);
      return std::nullopt;
    }

    lua_getfield(L, index, "Trim");
    bool trim = lua_toboolean(L,-1);
    lua_pop(L,1);

    lua_getfield(L, index, "TypicalResolution");
    auto typical_resolution_opt = tonumber(L,-1);
    lua_pop(L,1);

    if(!typical_resolution_opt) 
    {
      spdlog::get("cads")->error("{{ func = {},  msg = 'TypicalResolution not a number' }}",__func__);
      return std::nullopt;
    }

    return cads::GocatorConfig{trim,*typical_resolution_opt};
   
  }

  auto mk_conveyor(lua_State *L, int index) {

    cads::Conveyor obj;

    if(lua_istable(L,index)) {
      lua_getfield(L, index, "Id");
      obj.Id = lua_tonumber(L, -1);
      lua_pop(L, 1);

      lua_getfield(L, index, "Org");
      obj.Org = lua_tostring(L, -1);
      lua_pop(L, 1);

      lua_getfield(L, index, "Site");
      obj.Site = lua_tostring(L, -1);
      lua_pop(L, 1);

      lua_getfield(L, index, "Name");
      obj.Name = lua_tostring(L, -1);
      lua_pop(L, 1);

      lua_getfield(L, index, "Timezone");
      obj.Timezone = lua_tostring(L, -1);
      lua_pop(L, 1);

      lua_getfield(L, index, "PulleyCircumference");
      obj.PulleyCircumference =lua_tonumber(L, -1);
      lua_pop(L, 1);

      lua_getfield(L, index, "TypicalSpeed");
      obj.TypicalSpeed = lua_tonumber(L, -1);
      lua_pop(L, 1);

      lua_getfield(L, index, "Belt");
      obj.Belt = lua_tonumber(L, -1);
      lua_pop(L, 1);

      lua_getfield(L, index, "Length");
      obj.Length = lua_tonumber(L, -1);
      lua_pop(L, 1);
      
      lua_getfield(L, index, "WidthN");
      obj.WidthN = lua_tonumber(L, -1);
      lua_pop(L, 1);
    }

    return obj;
  }

  std::optional<cads::AnomalyDetection> mk_anomaly(lua_State *L, int index) {

    cads::AnomalyDetection obj;
    int isnum = 0; //false

    if(lua_istable(L,index)) {
      if(lua_getfield(L, index, "WindowSize") == LUA_TNUMBER) {
        obj.WindowSize = lua_tonumberx(L, -1, &isnum);
        lua_pop(L, 1);
        if(isnum == 0) goto ERR;
      } else goto ERR;

      if(lua_getfield(L, index, "BeltPartitionSize") == LUA_TNUMBER) {
        obj.BeltPartitionSize = lua_tonumberx(L, -1, &isnum);
        lua_pop(L, 1);
        if(isnum == 0) goto ERR;
      } else goto ERR;

      if(lua_getfield(L, index, "BeltSize") == LUA_TNUMBER) {
        obj.BeltSize = lua_tonumberx(L, -1, &isnum);
        lua_pop(L, 1);
        if(isnum == 0) goto ERR;
      } else goto ERR;

      if(lua_getfield(L, index, "MinPosition") == LUA_TNUMBER) {
        obj.MinPosition = lua_tonumberx(L, -1, &isnum);
        lua_pop(L, 1);
        if(isnum == 0) goto ERR;
      } else goto ERR;

      if(lua_getfield(L, index, "MaxPosition") == LUA_TNUMBER) {
        obj.MaxPosition = lua_tonumberx(L, -1, &isnum);
        lua_pop(L, 1);
        if(isnum == 0) goto ERR;
      } else goto ERR;

      if(lua_getfield(L, index, "ConveyorName") == LUA_TSTRING) {
        obj.ConveyorName = lua_tostring(L,-1);
        lua_pop(L, 1);
      } else goto ERR;

      return obj;
    }
    
    ERR:
    return std::nullopt;
  }

  int Io_gc(lua_State *L)
  {
    auto q = static_cast<cads::Io *>(lua_touserdata(L, 1));
    q->~Io();
    return 0;
  }

  int thread_gc(lua_State *L)
  {
    auto t = static_cast<std::thread *>(lua_touserdata(L, 1));
    if(t->joinable()) t->join();
    t->~thread();
    return 0;
  }

  int sleep_ms(lua_State *L) 
  {
    auto ms = lua_tointeger(L,1);
    std::this_thread::sleep_for(std::chrono::milliseconds(ms));
    return 0;
  }

  int join_threads(lua_State *L)
  {
    int array_length = lua_rawlen(L, -1);

    for (int i = 1; i <= array_length; i++) {
        lua_rawgeti(L, 1, i);
        if(lua_isuserdata(L,-1)) {
          auto v = lua_touserdata(L,-1);
          auto t = static_cast<std::thread*>(v);
          t->join();
        }else {
          spdlog::get("cads")->error("{{ func = {},  msg = 'Array contains non userdata' }}",__func__);
        }
        lua_pop(L, 1);
    }

    lua_pushnumber(L, 3);
    return 1;
  }

  int BlockingReaderWriterQueue(lua_State *L)
  {
      new (lua_newuserdata(L, sizeof(cads::Adapt<moodycamel::BlockingReaderWriterQueue<cads::msg>>))) cads::Adapt<moodycamel::BlockingReaderWriterQueue<cads::msg>> (moodycamel::BlockingReaderWriterQueue<cads::msg>());

      lua_createtable(L, 0, 1); 
      lua_pushcfunction(L, Io_gc);
      lua_setfield(L, -2, "__gc");
      lua_setmetatable(L, -2);

    return 1;
  }

  int wait_for(lua_State *L) {
    auto io = static_cast<cads::Io*>(lua_touserdata(L,1));
    auto s = lua_tointeger(L,2);

    cads::msg m;
    auto have_value = io->wait_dequeue_timed(m, std::chrono::seconds(s));
    lua_pushboolean(L,have_value);
    auto mid = std::get<0>(m);
    lua_pushinteger(L,mid);

    return 2;

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
    auto in = static_cast<cads::Io*>(lua_touserdata(L,-2));
    auto out = static_cast<cads::Io*>(lua_touserdata(L,-1));

    new (lua_newuserdata(L,sizeof(std::thread))) std::thread(fn,std::ref(*in),std::ref(*out));
    lua_createtable(L, 0, 1); 
    lua_pushcfunction(L, thread_gc);
    lua_setfield(L, -2, "__gc");
    lua_setmetatable(L, -2);
    return 1;
  }

  int gocator_start(lua_State *L) 
  {
    auto gocator = static_cast<std::unique_ptr<cads::GocatorReaderBase>*>(lua_touserdata(L,-2));
    auto fps = lua_tonumber(L,-1);
    (*gocator)->Start(fps);

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

  int sqlitegocator(lua_State *L) 
  {
    auto sqlite_gocator_config_opt = tosqlitegocatorconfig(L,1);

    if(!sqlite_gocator_config_opt)
    {
      lua_pushnil(L);
      return 1;
    }

    auto q = static_cast<cads::Io*>(lua_touserdata(L, 2));
    auto p = new (lua_newuserdata(L, sizeof(std::unique_ptr<cads::GocatorReaderBase>))) std::unique_ptr<cads::SqliteGocatorReader>;
    *p = std::make_unique<cads::SqliteGocatorReader>(*sqlite_gocator_config_opt,*q);

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

  int gocator(lua_State *L)
  {
    auto gocator_config_opt = togocatorconfig(L,1);

    if(!gocator_config_opt)
    {
      lua_pushnil(L);
      return 1;
    }

    auto q = static_cast<cads::Io*>(lua_touserdata(L, 2));
    auto p = new (lua_newuserdata(L, sizeof(std::unique_ptr<cads::GocatorReaderBase>))) std::unique_ptr<cads::GocatorReader>;
    *p = std::make_unique<cads::GocatorReader>(*gocator_config_opt,*q);

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
  
  int anomaly_detection_thread(lua_State *L)
  {
    using namespace std::placeholders;
    auto anomaly = mk_anomaly(L,1);
    
    if(anomaly) {
      auto bound = std::bind(cads::splice_detection_thread,*anomaly,_1,_2);
      mk_thread2(L,bound);
      lua_pushboolean(L,0);
    }else {
      lua_pushnil(L);
      lua_pushboolean(L,1);
    }

    return 2;
  }

  int window_processing_thread(lua_State *L) 
  {
    return mk_thread2(L,cads::window_processing_thread);
  }

  int dynamic_processing_thread(lua_State *L) 
  {
    return mk_thread2(L,cads::dynamic_processing_thread);
  }

  int save_send_thread(lua_State *L) {
    using namespace std::placeholders;

    auto conveyor = mk_conveyor(L,1);
    auto bound = std::bind(cads::save_send_thread,conveyor,_1,_2);
    return mk_thread2(L,bound);
  }

  int loop_beltlength_thread(lua_State *L) {
    using namespace std::placeholders;

    auto conveyor = mk_conveyor(L,1);
    auto bound = std::bind(cads::loop_beltlength_thread,conveyor,_1,_2);
    return mk_thread2(L,bound);
  }

  int process_profile(lua_State *L) {
    return mk_thread2(L,cads::process_profile);
  }

  int process_identity(lua_State *L) {
    return mk_thread2(L,cads::process_identity);
  }

  int encoder_distance_estimation(lua_State *L) {

    auto next = static_cast<cads::Io*>(lua_touserdata(L,1));
    double stride = lua_tonumber(L,2);
    new (lua_newuserdata(L, sizeof(cads::Adapt<decltype(cads::encoder_distance_estimation(std::ref(*next),stride))>))) cads::Adapt<decltype(cads::encoder_distance_estimation(std::ref(*next),stride))>(cads::encoder_distance_estimation(std::ref(*next),stride));

    lua_createtable(L, 0, 1); 
    lua_pushcfunction(L, Io_gc);
    lua_setfield(L, -2, "__gc");
    lua_setmetatable(L, -2);
    return 1;

  }

  int get_serial(lua_State *L) {
    lua_pushinteger(L, cads::constants_device.Serial);
    return 1;
  }

}

namespace cads
{
  namespace lua
  {
    
    Lua init(Lua UL)
    {
      auto L = UL.get();

      lua_pushcfunction(L, ::BlockingReaderWriterQueue);
	    lua_setglobal(L,"BlockingReaderWriterQueue");

      lua_pushcfunction(L, ::anomaly_detection_thread);
	    lua_setglobal(L,"anomaly_detection_thread");

      lua_pushcfunction(L, ::save_send_thread);
	    lua_setglobal(L,"save_send_thread");

      lua_pushcfunction(L, ::window_processing_thread);
	    lua_setglobal(L,"window_processing_thread");

      lua_pushcfunction(L, ::loop_beltlength_thread);
	    lua_setglobal(L,"loop_beltlength_thread");

      
      lua_pushcfunction(L, ::dynamic_processing_thread);
	    lua_setglobal(L,"dynamic_processing_thread");
      
      lua_pushcfunction(L, ::wait_for);
      lua_setglobal(L,"wait_for");

      lua_pushcfunction(L, ::join_threads);
      lua_setglobal(L,"join_threads");

      lua_pushcfunction(L, ::gocator);
      lua_setglobal(L,"gocator");

      lua_pushcfunction(L, ::sqlitegocator);
      lua_setglobal(L,"sqlitegocator");

      lua_pushcfunction(L, ::process_profile);
      lua_setglobal(L,"process_profile");
      
      lua_pushcfunction(L, ::process_identity);
      lua_setglobal(L,"process_identity");

      lua_pushcfunction(L, ::encoder_distance_estimation);
      lua_setglobal(L,"encoder_distance_estimation");

      lua_pushcfunction(L, ::sleep_ms);
      lua_setglobal(L,"sleep_ms");

      lua_pushcfunction(L, ::get_serial);
      lua_setglobal(L,"get_serial");
      
      
      return UL;
    }

  }


  std::tuple<Lua,bool> run_lua_code(std::string lua_code) {

    auto L = Lua{luaL_newstate(),lua_close};
    luaL_openlibs( L.get() );

    L = lua::init(std::move(L));
    auto lua_status = luaL_dostring(L.get(),lua_code.c_str());

    if(lua_status != LUA_OK) {
      spdlog::get("cads")->error("{{ func = {},  msg = 'luaL_dofile: {}' }}",__func__,lua_tostring(L.get(),-1));
      return {std::move(L),true};
    }

    return {std::move(L),false};
  }

  std::tuple<Lua,bool> run_lua_config(std::string f) 
  {
    namespace fs = std::filesystem;

    fs::path luafile{f};
    luafile.replace_extension("lua");

    auto L = Lua{luaL_newstate(),lua_close};
    luaL_openlibs( L.get() );
    
    L = lua::init(std::move(L));

    auto lua_status = luaL_dofile(L.get(),luafile.string().c_str());
    
    if(lua_status != LUA_OK) {
      spdlog::get("cads")->error("{}: luaL_dofile: {}",__func__,lua_tostring(L.get(),-1));
      return {std::move(L),true};
    }

    return {std::move(L),false};
  }

}