
#include <lua.hpp>

#include <measurements.h>
#include <coms.h>
#include <utils.hpp>

namespace {
  
  int send_metrics(lua_State *L) {
    auto p = (cads::coro<int, std::tuple<std::string, std::string>, 1> *)lua_topointer(L, 1);
    std::string sub(lua_tostring(L,2));
    std::string msg(lua_tostring(L,3));
    p->resume({sub,msg});
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

}


namespace cads {


  void measurement_thread(moodycamel::BlockingReaderWriterQueue<Measure::MeasureMsg> &measure, bool &terminate)
  {
    auto realtime_metrics = realtime_metrics_coro();
    
    auto lua = luaL_newstate();
    luaL_openlibs( lua );
    luaL_dofile(lua,"../measurements.lua");

    luaL_Reg fields[]{
      {"__call", send_metrics},
      {NULL,NULL}};

    luaL_newmetatable(lua, "cads.out");
    luaL_setfuncs(lua, fields, 0);
    lua_pushlightuserdata(lua, &realtime_metrics);
    luaL_setmetatable(lua, "cads.out");
    lua_setglobal(lua, "out");

    lua_pushcfunction(lua,time_str);
    lua_setglobal(lua,"timeToString");

    lua_getglobal(lua,"make");
    auto tp = std::chrono::duration<double>(date::utc_clock::now().time_since_epoch());
    double tpd = tp.count();
    lua_pushnumber(lua, tpd);
    lua_pcall(lua, 1, 0, 0);
      
    Measure::MeasureMsg m;
    for (;!terminate;)
    {
      if (!measure.wait_dequeue_timed(m, std::chrono::milliseconds(1000)))
      {
        continue; // graceful thread terminate
      }
      
      lua_getglobal(lua,"send");
      lua_pushstring(lua,get<0>(m).c_str());
      
      switch (get<1>(m).index()) {
        case Measure::MeasureType::mDouble:
          lua_pushnumber(lua, get<double>(get<1>(m)));      
          break;
        case Measure::MeasureType::mString:
          lua_pushstring(lua, get<std::string>(get<1>(m)).c_str());
          break;
        default: break;
      }

      tp = std::chrono::duration<double>(get<2>(m).time_since_epoch());
      lua_pushnumber(lua, tp.count());
      lua_pcall(lua, 3, 0, 0);

    }
  }


  void Measure::init() {
    thread = std::jthread(measurement_thread,std::ref(fifo),std::ref(terminate));
  }

  Measure::~Measure() {
    terminate = true;
    thread.join();
  }
  
  void Measure::send(std::string measure, double value) {
    fifo.enqueue({measure,value,date::utc_clock::now()});
  }

  void Measure::send(std::string measure, std::string value) {
    fifo.enqueue({measure,value,date::utc_clock::now()});
  }
}