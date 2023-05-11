
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
    auto realtime_metrics = realtime_metrics_coro();
    
    auto lua = luaL_newstate();
    luaL_openlibs( lua );
    int lua_status = luaL_dofile(lua,"../measurements.lua");
    
    if(lua_status != LUA_OK) {
      spdlog::get("cads")->error("{}:luaL_dofile {}",__func__,lua_status);
    }

    lua_pushlightuserdata(lua, &realtime_metrics);
    lua_pushcclosure(lua, send_metrics, 1);
    lua_setglobal(lua, "out");

    lua_pushcfunction(lua,time_str);
    lua_setglobal(lua,"timeToString");

    lua_getglobal(lua,"make");
    auto tp = std::chrono::duration<double>(date::utc_clock::now().time_since_epoch());
    double tpd = tp.count();
    lua_pushnumber(lua, tpd);
    lua_status = lua_pcall(lua, 1, 0, 0);

    if(lua_status != LUA_OK) {
      spdlog::get("cads")->error("{}:lua_pcall {}",__func__,lua_status);
    }

    Measure::MeasureMsg m;
    for (;!terminate;)
    {
      if (!measure.wait_dequeue_timed(m, std::chrono::milliseconds(1000)))
      {
        continue; // graceful thread terminate
      }
      
      auto [sub,quality,time,value] = m;
      lua_getglobal(lua,"send");
      lua_pushstring(lua,sub.c_str());
      lua_pushnumber(lua,quality);
      tp = std::chrono::duration<double>(get<2>(m).time_since_epoch());
      lua_pushnumber(lua, tp.count());
      int param_number = 4;
      
      switch (value.index()) {
        case 0:
          lua_pushnumber(lua, get<double>(value));      
          break;
        case 1:
          lua_pushstring(lua, get<std::string>(value).c_str());
          break;
        case 2: 
          lua_pushlightuserdata(lua, &value);
          lua_pushcclosure(lua, execute_func2<double>, 1);
          break;
         case 3: 
          lua_pushlightuserdata(lua, &value);
          lua_pushcclosure(lua, execute_func2<std::string>, 1);
          break;
        case 4: {
          auto [v,location] = get<std::tuple<double,double>>(value);
          lua_pushnumber(lua, v); 
          lua_pushnumber(lua, location); 
          param_number++;
          break;
        }
        default: break;
      }

      lua_status = lua_pcall(lua, param_number, 0, 0);
      
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
    thread.join();
  }
  
  void Measure::send(std::string measure, int quality, double value) {
    fifo.enqueue({measure,quality,date::utc_clock::now(),value});
  }

  void Measure::send(std::string measure, int quality, std::string value) {
    fifo.enqueue({measure,quality,date::utc_clock::now(),value});
  }
  
  void Measure::send(std::string measure, int quality, std::function<double()> value) {
    fifo.enqueue({measure,quality,date::utc_clock::now(),value});
  }

  void Measure::send(std::string measure, int quality, std::function<std::string()> value) {
    fifo.enqueue({measure,quality,date::utc_clock::now(),value});
  }

  void Measure::send(std::string measure, int quality, std::tuple<double,double> value) {
    fifo.enqueue({measure,quality,date::utc_clock::now(),value});
  }
}