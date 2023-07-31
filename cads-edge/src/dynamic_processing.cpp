#include <vector>
#include <cstring>
#include <chrono>
#include <unordered_set>
#include <future>

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wstringop-overflow="
#include <lua.hpp>
#include <spdlog/spdlog.h>

#pragma GCC diagnostic pop


#include <dynamic_processing.h>
#include <constants.h>
#include <msg.h>
#include <coro.hpp>
#include <coms.h>

using namespace std;
using namespace moodycamel;
using namespace std::chrono;

namespace cads
{

  static int array_index(lua_State *L)
  {
    auto *p = (z_element *)lua_topointer(L, 1);
    auto index = lua_tointeger(L, 2);
    lua_pushnumber(L, *(p + index - 1));
    return 1;
  }

  static int array_newindex(lua_State *L)
  {
    auto *p = (z_element *)lua_topointer(L, 1);
    auto index = lua_tointeger(L, 2);
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

    luaL_newmetatable(L, "cads.window");
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
    
    if(width < 1 || height < 1) {
      std::throw_with_nested(std::runtime_error("lua_processing_coro: Width or height less than one"));
    }
    
    auto window = std::vector<z_element>(size_t(width * height), 33.0/*std::numeric_limits<z_element>::quiet_NaN()*/);

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
    std::unordered_set<double> anomolies;

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

        if((int)p.z.size() != width) {
          spdlog::get("cads")->error("Profile width {} != window width {}",p.z.size(),width);
          break;
        }
        memmove(window.data() + width, window.data(), size_t(width*(height-1))*sizeof(z_element)); // shift 2d array by one row
        memcpy(window.data(), p.z.data(), size_t(width)*sizeof(z_element));
        
        result = 0.0;
        auto belt_section = cnt++ % (int64_t)height;
        if(belt_section == 0) {
          result = eval_lua_process(L,width,height);
          auto location = std::round(p.y / 1000) * 1000;
          if(result > 0 /*&& !anomolies.contains(location)*/) {
            measurements.send("anomaly",0,std::make_tuple(result,location));
            //anomolies.insert(location);
          }
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

  void dynamic_processing_thread(DynamicProcessingConfig config, cads::Io &profile_fifo, cads::Io &next_fifo)
  {

    int64_t cnt = 0;
    profile p;
    cads::msg m;
    auto buffer_size_warning = buffer_warning_increment;
    int widthn = (int)config.WidthN;

    auto realtime_processing = lua_processing_coro(widthn);
    
    auto start = std::chrono::high_resolution_clock::now();

    bool module_failure = false;

    for (auto loop = true;loop;)
    {
      ++cnt;
      profile_fifo.wait_dequeue(m);

      switch(get<0>(m)) {
        case msgid::scan:
           p = get<profile>(get<1>(m));
           next_fifo.enqueue(m);
        break;
        case msgid::finished:
          loop = false;
          next_fifo.enqueue(m);
          continue;
        default: // Passthrough
          next_fifo.enqueue(m);
          continue;
      }

      // Stops filling up logs with errors
      if(realtime_processing.is_done()) {
        continue;
      }

      auto [err, rslt] = realtime_processing.resume(m);

      if (err)
      {
        module_failure = true;
        spdlog::get("cads")->error(R"({{func = '{}' fn = '{}', rtn = {}, msg = '{}' }})",__func__,"realtime_processing",err,"Stopped processing. Passthrough only");
      }

      if (rslt > 0)
      {
        spdlog::get("cads")->debug("Belt damage found around y: {}", p.y);
      }

      if (profile_fifo.size_approx() > buffer_size_warning)
      {
        spdlog::get("cads")->warn("Cads Dynamic Processing showing signs of not being able to keep up with data source. Size {}", buffer_size_warning);
        buffer_size_warning += buffer_warning_increment;
      }
    }

    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();

    next_fifo.enqueue({msgid::finished, 0});
    realtime_processing.terminate();

    auto rate = duration != 0 ? (double)cnt / duration : 0;
    spdlog::get("cads")->info("DYNAMIC PROCESSING - CNT: {}, DUR: {}, RATE(ms):{} ", cnt, duration, rate);
  }

}