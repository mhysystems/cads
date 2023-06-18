
#include <type_traits>

#include <lua.hpp>

#include <measurements.h>
#include <coms.h>
#include <utils.hpp>
#include <spdlog/spdlog.h>

namespace {
  
  int send_metrics(lua_State *L) {
    auto p = (cads::coro<int, std::tuple<std::string, std::string, std::string>, 1> *)lua_topointer(L, lua_upvalueindex(1));
    std::string sub(lua_tostring(L,1));
    std::string cat(lua_tostring(L,2));
    std::string msg(lua_tostring(L,3));
    p->resume({sub,cat,msg});
    return 0;
  }

  int time_str(lua_State *L) {
    using ds = std::chrono::duration<double>;
    auto time = lua_tonumber(L,1);
    ds ddd(time);
    auto tp = date::utc_time(ddd);
    auto str = date::format("%FT%TZ", tp);
    lua_pushstring(L,str.c_str());
    return 1;
  }

  int execute_func(lua_State *L) {
    auto p = (std::function<double()> *)lua_topointer(L, lua_upvalueindex(1));
    auto r = (*p)();
    lua_pushnumber(L,r);
    return 1;
  }

  template<class T>int execute_func2(lua_State *L) {
    auto p = (std::function<T()> *)lua_topointer(L, lua_upvalueindex(1));
    auto r = (*p)();
    
    if constexpr (std::is_same<double,T>::value) {
      lua_pushnumber(L,r);    
    }else{
      lua_pushstring(L,r.c_str()); 
    }

    return 1;
  }

}


namespace cads {


  void measurement_thread(moodycamel::BlockingConcurrentQueue<Measure::MeasureMsg> &measure, bool &terminate)
  {
    using Lua = std::unique_ptr<lua_State, decltype(&lua_close)>;
    auto realtime_metrics = realtime_metrics_coro();
    
    auto consume_fifo_onerror = [&]() {
      for (;!terminate;)
      {  
        Measure::MeasureMsg m;
      
        if (!measure.wait_dequeue_timed(m, std::chrono::milliseconds(1000)))
        {
          continue; // graceful thread terminate
        }
      }
    };

    auto L = Lua{luaL_newstate(),lua_close};
    luaL_openlibs( L.get() );

    int lua_status = luaL_dofile(L.get(),"../measurements.lua");
    
    if(lua_status != LUA_OK) {
      spdlog::get("cads")->error("{}:luaL_dofile {}",__func__,lua_status);
      return consume_fifo_onerror();
    }

    lua_pushlightuserdata(L.get(), &realtime_metrics);
    lua_pushcclosure(L.get(), send_metrics, 1);
    lua_setglobal(L.get(), "out");

    lua_pushcfunction(L.get(),time_str);
    lua_setglobal(L.get(),"timeToString");

    lua_getglobal(L.get(),"make");
    auto tp = std::chrono::duration<double>(date::utc_clock::now().time_since_epoch());
    double tpd = tp.count();
    lua_pushnumber(L.get(), tpd);
    lua_status = lua_pcall(L.get(), 1, 0, 0);

    if(lua_status != LUA_OK) {
      spdlog::get("cads")->error("{}:lua_pcall {}",__func__,lua_status);
      return consume_fifo_onerror();
    }
  
    for (;!terminate;)
    {  
      Measure::MeasureMsg m;
      
      if (!measure.wait_dequeue_timed(m, std::chrono::milliseconds(1000)))
      {
        continue; // graceful thread terminate
      }
      
      auto [sub,quality,time,value] = m;
      lua_getglobal(L.get(),"send");
      lua_pushstring(L.get(),sub.c_str());
      lua_pushnumber(L.get(),quality);
      tp = std::chrono::duration<double>(get<2>(m).time_since_epoch());
      lua_pushnumber(L.get(), tp.count());
      int param_number = 4;
      
      switch (value.index()) {
        case 0:
          lua_pushnumber(L.get(), get<double>(value));      
          break;
        case 1:
          lua_pushstring(L.get(), get<std::string>(value).c_str());
          break;
        case 2: 
          lua_pushlightuserdata(L.get(), &value);
          lua_pushcclosure(L.get(), execute_func2<double>, 1);
          break;
         case 3: 
          lua_pushlightuserdata(L.get(), &value);
          lua_pushcclosure(L.get(), execute_func2<std::string>, 1);
          break;
        case 4: {
          auto [v,location] = get<std::tuple<double,double>>(value);
          lua_pushnumber(L.get(), v); 
          lua_pushnumber(L.get(), location); 
          param_number++;
          break;
        }
        default: break;
      }

      lua_status = lua_pcall(L.get(), param_number, 0, 0);
      
      if(lua_status != LUA_OK) {
        spdlog::get("cads")->error("{}:lua_pcall {}",__func__,lua_status);
      }

    }
  }


  void Measure::init() {
    thread = std::jthread(measurement_thread,std::ref(fifo),std::ref(terminate));
  }

  Measure::~Measure() {
    terminate = true;
    if(thread.joinable()) thread.join();
  }
  
  void Measure::send(std::string measure, int quality, double value) {
    fifo.try_enqueue({measure,quality,date::utc_clock::now(),value});
  }

  void Measure::send(std::string measure, int quality, std::string value) {
    fifo.try_enqueue({measure,quality,date::utc_clock::now(),value});
  }
  
  void Measure::send(std::string measure, int quality, std::function<double()> value) {
    fifo.try_enqueue({measure,quality,date::utc_clock::now(),value});
  }

  void Measure::send(std::string measure, int quality, std::function<std::string()> value) {
    fifo.try_enqueue({measure,quality,date::utc_clock::now(),value});
  }

  void Measure::send(std::string measure, int quality, std::tuple<double,double> value) {
    fifo.try_enqueue({measure,quality,date::utc_clock::now(),value});
  }
}