#include <thread>
#include <filesystem>
#include <memory>
#include <string>

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
#include <filters.h>
#include <init.h>
#include <fiducial.h>
#include <utils.hpp>
#include <scandb.h>

namespace
{
  using namespace std::string_literals;

  template <typename R, typename T>
  auto TransformMsg(std::function<R(T)> fn, cads::Io<R> *io)
  {
    return [io, fn](T m)
    { return io->enqueue(fn(m)); };
  }

  auto FilterMsg(cads::msgid id, cads::Io<cads::msg> *io)
  {
    return [io, id](cads::msg m)
    {
      auto m_id = get<0>(m);
      if (m_id == id)
      {
        return io->enqueue(m);
      }
      else
      {
        return true;
      }
    };
  }

  int time_str(lua_State *L)
  {
    using ds = std::chrono::duration<double>;
    auto time = lua_tonumber(L, 1);
    ds ddd(time);
    auto tp = date::utc_time(ddd);
    auto str = date::format("%FT%TZ", tp);
    lua_pushstring(L, str.c_str());
    return 1;
  }

  int get_now(lua_State *L)
  {
    auto tp = std::chrono::duration<double>(date::utc_clock::now().time_since_epoch());
    double tpd = tp.count();
    lua_pushnumber(L, tpd);
    return 1;
  }

  int send_external_msg(lua_State *L)
  {
    auto p = (moodycamel::BlockingConcurrentQueue<std::tuple<std::string, std::string, std::string>> *)lua_topointer(L, lua_upvalueindex(1));
    size_t strlen = 0;
    auto str = lua_tolstring(L, 1, &strlen);
    std::string sub(str, strlen);
    str = lua_tolstring(L, 2, &strlen);
    std::string cat(str, strlen);
    str = lua_tolstring(L, 3, &strlen);
    std::string msg(str, strlen);
    p->enqueue({sub, cat, msg});
    return 0;
  }

  int execute_func(lua_State *L)
  {
    auto p = (std::function<double()> *)lua_topointer(L, lua_upvalueindex(1));
    auto r = (*p)();
    lua_pushnumber(L, r);
    return 1;
  }

  template <class T>
  int execute_func2(lua_State *L)
  {
    auto p = (std::function<T()> *)lua_topointer(L, lua_upvalueindex(1));
    auto r = (*p)();

    if constexpr (std::is_same<double, T>::value)
    {
      lua_pushnumber(L, r);
    }
    else
    {
      lua_pushstring(L, r.c_str());
    }

    return 1;
  }

  std::optional<std::string> tostring(lua_State *L, int index)
  {
    size_t len = 0;

    if (!lua_isstring(L, index))
    {
      spdlog::get("cads")->error("{{ func = {},  msg = 'lua object not a string' }}", __func__);
      return std::nullopt;
    }

    auto a = lua_tolstring(L, index, &len);

    return a;
  }

  std::optional<double> tonumber(lua_State *L, int index)
  {
    int isnum = 0;
    auto a = lua_tonumberx(L, index, &isnum);

    if (isnum == 0)
    {
      spdlog::get("cads")->error("{{ func = {},  msg = 'lua object not a number' }}", __func__);
      return std::nullopt;
    }

    return a;
  }

  std::optional<long long> tointeger(lua_State *L, int index)
  {
    int isnum = 0;
    auto a = lua_tointegerx(L, index, &isnum);

    if (isnum == 0)
    {
      spdlog::get("cads")->error("{{ func = {},  msg = 'lua object not a integer' }}", __func__);
      return std::nullopt;
    }

    return a;
  }

  std::optional<std::vector<double>> tonumbervector(lua_State *L, int index)
  {
    if (!lua_istable(L, index))
    {
      spdlog::get("cads")->error("{{ func = {},  msg = 'lua object needs to be a array' }}", __func__);
      return std::nullopt;
    }

    auto array_length = lua_rawlen(L, index);
    std::vector<double> lua_array;

    for (decltype(array_length) i = 1; i <= array_length; i++)
    {
      lua_rawgeti(L, -1, i);
      auto num_opt = tonumber(L, -1);
      lua_pop(L, 1);

      if (!num_opt)
      {
        spdlog::get("cads")->error("{{ func = {},  msg = 'Index {} not a number' }}", __func__, i);
        return std::nullopt;
      }
      else
      {
        lua_array.push_back(*num_opt);
      }
    }

    return lua_array;
  }

  std::optional<std::vector<std::vector<double>>> to2Dnumbervector(lua_State *L, int index)
  {
    if (!lua_istable(L, index))
    {
      spdlog::get("cads")->error("{{ func = {},  msg = 'lua object needs to be a array' }}", __func__);
      return std::nullopt;
    }

    auto array_length = lua_rawlen(L, index);
    std::vector<std::vector<double>> lua_array;

    for (decltype(array_length) i = 1; i <= array_length; i++)
    {
      lua_rawgeti(L, -1, i);
      auto vec = tonumbervector(L, -1);
      lua_pop(L, 1);

      if (!vec)
      {
        spdlog::get("cads")->error("{{ func = {},  msg = 'Index {} not an array' }}", __func__, i);
        return std::nullopt;
      }
      else
      {
        lua_array.push_back(*vec);
      }
    }

    return lua_array;
  }

  template <typename T1, typename T2>
  std::optional<std::tuple<T1, T2>> topair(lua_State *L, int index);

  template <>
  std::optional<std::tuple<long long, long long>> topair(lua_State *L, int index)
  {
    if (!lua_istable(L, index))
    {
      spdlog::get("cads")->error("{{ func = {},  msg = 'lua object needs to be a array' }}", __func__);
      return std::nullopt;
    }

    int array_length = lua_rawlen(L, index);

    if (array_length != 2)
    {
      spdlog::get("cads")->error("{{ func = {},  msg = 'lua object should have only 2 elements in array' }}", __func__);
      return std::nullopt;
    }

    int num = 0;
    if (lua_rawgeti(L, index, 1) != LUA_TNUMBER)
    {
      spdlog::get("cads")->error("{{ func = {},  msg = 'lua array doesn't contain number at index {}' }}", __func__, 1);
      return std::nullopt;
    }

    auto a = lua_tointegerx(L, -1, &num);
    lua_pop(L, 1);

    if (num == 0)
    {
      spdlog::get("cads")->error("{{ func = {},  msg = 'lua array doesn't contain integer at index {}' }}", __func__, 1);
      return std::nullopt;
    }

    if (lua_rawgeti(L, index, 2) != LUA_TNUMBER)
    {
      spdlog::get("cads")->error("{{ func = {},  msg = 'lua array doesn't contain number at index {}' }}", __func__, 2);
      return std::nullopt;
    }

    auto b = lua_tointegerx(L, -1, &num);
    lua_pop(L, 1);

    if (num == 0)
    {
      spdlog::get("cads")->error("{{ func = {},  msg = 'lua array doesn't contain integer at index {}' }}", __func__, 2);
      return std::nullopt;
    }

    return std::make_tuple(a, b);
  }

  template<class R> std::optional<R> tofield(lua_State *L, int index, std::string obj_name, std::string field, std::function<std::optional<R>(lua_State*,int)> fieldfn)
  {
    if (lua_getfield(L, index, field.c_str()) == LUA_TNIL)
    {
      spdlog::get("cads")->error("{{ func = {},  msg = '{} requires {}' }}", __func__, obj_name, field);
      return std::nullopt;
    }

    auto field_opt = fieldfn(L, -1);
    lua_pop(L, 1);

    if (!field_opt)
    {
      spdlog::get("cads")->error("{{ func = {},  msg = '{} not a number' }}", __func__, field);
      return std::nullopt;
    }

    return field_opt;
  }

  std::optional<double> tofieldnumber(lua_State *L, int index, std::string obj_name, std::string field)
  {
    if (lua_getfield(L, index, field.c_str()) == LUA_TNIL)
    {
      spdlog::get("cads")->error("{{ func = {},  msg = '{} requires {}' }}", __func__, obj_name, field);
      return std::nullopt;
    }

    auto field_opt = tonumber(L, -1);
    lua_pop(L, 1);

    if (!field_opt)
    {
      spdlog::get("cads")->error("{{ func = {},  msg = '{} not a number' }}", __func__, field);
      return std::nullopt;
    }

    return field_opt;
  }

  std::optional<std::string> tofieldstring(lua_State *L, int index, std::string obj_name, std::string field)
  {
    if (lua_getfield(L, index, field.c_str()) == LUA_TNIL)
    {
      spdlog::get("cads")->error("{{ func = {},  msg = '{} requires {}' }}", __func__, obj_name, field);
      return std::nullopt;
    }

    auto field_opt = tostring(L, -1);
    lua_pop(L, 1);

    if (!field_opt)
    {
      spdlog::get("cads")->error("{{ func = {},  msg = '{} not a number' }}", __func__, field);
      return std::nullopt;
    }

    return field_opt;
  }

  std::optional<cads::Conveyor> toconveyor(lua_State *L, int index)
  {

    const std::string obj_name = "conveyor";

    if (!lua_istable(L, index))
    {
      spdlog::get("cads")->error("{{ func = {},  msg = '{} needs to be a table' }}", __func__, obj_name);
      return std::nullopt;
    }

    if (lua_getfield(L, index, "Site") == LUA_TNIL)
    {
      spdlog::get("cads")->error("{{ func = {},  msg = '{} requires {}' }}", __func__, obj_name, "Site");
      return std::nullopt;
    }

    auto site_opt = tostring(L, -1);
    lua_pop(L, 1);

    if (!site_opt)
    {
      spdlog::get("cads")->error("{{ func = {},  msg = 'Site not a string' }}", __func__);
      return std::nullopt;
    }

    if (lua_getfield(L, index, "Name") == LUA_TNIL)
    {
      spdlog::get("cads")->error("{{ func = {},  msg = '{} requires {}' }}", __func__, obj_name, "Name");
      return std::nullopt;
    }

    auto name_opt = tostring(L, -1);
    lua_pop(L, 1);

    if (!name_opt)
    {
      spdlog::get("cads")->error("{{ func = {},  msg = 'Name not a string' }}", __func__);
      return std::nullopt;
    }

    if (lua_getfield(L, index, "Timezone") == LUA_TNIL)
    {
      spdlog::get("cads")->error("{{ func = {},  msg = '{} requires {}' }}", __func__, obj_name, "Timezone");
      return std::nullopt;
    }

    auto Timezone_opt = tostring(L, -1);
    lua_pop(L, 1);

    if (!Timezone_opt)
    {
      spdlog::get("cads")->error("{{ func = {},  msg = 'Timezone not a string' }}", __func__);
      return std::nullopt;
    }

    if (lua_getfield(L, index, "PulleyCircumference") == LUA_TNIL)
    {
      spdlog::get("cads")->error("{{ func = {},  msg = '{} requires {}' }}", __func__, obj_name, "PulleyCircumference");
      return std::nullopt;
    }

    auto PulleyCircumference_opt = tonumber(L, -1);
    lua_pop(L, 1);

    if (!PulleyCircumference_opt)
    {
      spdlog::get("cads")->error("{{ func = {},  msg = 'PulleyCircumference not a number' }}", __func__);
      return std::nullopt;
    }

    if (lua_getfield(L, index, "TypicalSpeed") == LUA_TNIL)
    {
      spdlog::get("cads")->error("{{ func = {},  msg = '{} requires {}' }}", __func__, obj_name, "TypicalSpeed");
      return std::nullopt;
    }

    auto TypicalSpeed_opt = tonumber(L, -1);
    lua_pop(L, 1);

    if (!TypicalSpeed_opt)
    {
      spdlog::get("cads")->error("{{ func = {},  msg = 'TypicalSpeed not a number' }}", __func__);
      return std::nullopt;
    }

    return cads::Conveyor{*site_opt, *name_opt, *Timezone_opt, *PulleyCircumference_opt, *TypicalSpeed_opt};
  }

std::optional<cads::Belt> tobelt(lua_State *L, int index)
  {

    const std::string obj_name = "belt";

    if (!lua_istable(L, index))
    {
      spdlog::get("cads")->error("{{ func = {},  msg = '{} needs to be a table' }}", __func__, obj_name);
      return std::nullopt;
    }

    auto Serial_opt = tofieldstring(L, index, obj_name,"Serial"s);

    if (!Serial_opt)
    {
      return std::nullopt;
    }

    auto PulleyCover_opt = tofieldnumber(L, index, obj_name, "PulleyCover"s);

    if (!PulleyCover_opt)
    {
      return std::nullopt;
    }

    auto CordDiameter_opt = tofieldnumber(L, index, obj_name, "CordDiameter"s);

    if (!CordDiameter_opt)
    {
      return std::nullopt;
    }
    
    auto TopCover_opt = tofieldnumber(L, index, obj_name, "TopCover"s);

    if (!TopCover_opt)
    {
      return std::nullopt;
    }

    auto Length_opt = tofieldnumber(L, index, obj_name, "Length"s);

    if (!Length_opt)
    {
      return std::nullopt;
    }

    auto Width_opt = tofieldnumber(L, index, obj_name, "Width"s);

    if (!Width_opt)
    {
      return std::nullopt;
    }

    auto WidthN_opt = tofieldnumber(L, index, obj_name, "WidthN"s);

    if (!WidthN_opt)
    {
      return std::nullopt;
    }
    return cads::Belt{*Serial_opt, *PulleyCover_opt, *CordDiameter_opt, *TopCover_opt, *Length_opt, *Width_opt, *WidthN_opt};
  }

  std::optional<cads::ScanMeta>  toscanmeta(lua_State *L, int index) 
  {
    const std::string obj_name = "scanmeta";

    if (!lua_istable(L, index))
    {
      spdlog::get("cads")->error("{{ func = {},  msg = '{} needs to be a table' }}", __func__, obj_name);
      return std::nullopt;
    }

    auto Version_opt = tofield<int64_t>(L, index, obj_name,"Version"s, tointeger);

    if (!Version_opt)
    {
      return std::nullopt;
    }

    auto ZEncoding_opt = tofield<int64_t>(L, index, obj_name,"ZEncoding"s, tointeger);

    if (!ZEncoding_opt)
    {
      return std::nullopt;
    }

    return cads::ScanMeta{*Version_opt,*ZEncoding_opt};

  }


  std::optional<cads::ScanStorageConfig> toscanstorageconfig(lua_State *L, int index)
  {

    const std::string obj_name = "scanstorageconfig";

    if (!lua_istable(L, index))
    {
      spdlog::get("cads")->error("{{ func = {},  msg = '{} needs to be a table' }}", __func__, obj_name);
      return std::nullopt;
    }

    auto Belt_opt = tofield<cads::Belt>(L, index, obj_name,"Belt"s,tobelt);

    if (!Belt_opt)
    {
      return std::nullopt;
    }

    auto Conveyor_opt = tofield<cads::Conveyor>(L, index, obj_name, "Conveyor"s,toconveyor);

    if (!Conveyor_opt)
    {
      return std::nullopt;
    }

    auto ScanMeta_opt = tofield<cads::ScanMeta>(L, index, obj_name, "ScanMeta"s,toscanmeta);

    if (!ScanMeta_opt)
    {
      return std::nullopt;
    }
    
    auto RegisterUpload_opt = tofield<bool>(L, index, obj_name,"RegisterUpload"s,lua_toboolean);

    if (!RegisterUpload_opt)
    {
      return std::nullopt;
    }

    return cads::ScanStorageConfig{.belt = *Belt_opt, .conveyor = *Conveyor_opt, .meta = *ScanMeta_opt, .register_upload = *RegisterUpload_opt};
  }


  std::optional<cads::Dbscan> todbscan(lua_State *L, int index)
  {
    const std::string obj_name = "dbscan";

    if (!lua_istable(L, index))
    {
      spdlog::get("cads")->error("{{ func = {},  msg = '{} needs to be a table' }}", __func__, obj_name);
      return std::nullopt;
    }

    if (lua_getfield(L, index, "InClusterRadius") == LUA_TNIL)
    {
      spdlog::get("cads")->error("{{ func = {},  msg = '{} requires {}' }}", __func__, obj_name, "InClusterRadius");
      return std::nullopt;
    }

    auto InClusterRadius_opt = tonumber(L, -1);
    lua_pop(L, 1);

    if (!InClusterRadius_opt)
    {
      spdlog::get("cads")->error("{{ func = {},  msg = 'InClusterRadius not a number' }}", __func__);
      return std::nullopt;
    }

    if (lua_getfield(L, index, "MinPoints") == LUA_TNIL)
    {
      spdlog::get("cads")->error("{{ func = {},  msg = '{} requires {}' }}", __func__, obj_name, "MinPoints");
      return std::nullopt;
    }

    auto MinPoints_opt = tointeger(L, -1);
    lua_pop(L, 1);

    if (!MinPoints_opt)
    {
      spdlog::get("cads")->error("{{ func = {},  msg = 'MinPoints not a number' }}", __func__);
      return std::nullopt;
    }

    if (*MinPoints_opt < 0)
    {
      spdlog::get("cads")->error("{{ func = {},  msg = 'MinPoints is less than zero' }}", __func__);
      return std::nullopt;
    }

    if (lua_getfield(L, index, "ZMergeRadius") == LUA_TNIL)
    {
      spdlog::get("cads")->error("{{ func = {},  msg = '{} requires {}' }}", __func__, obj_name, "ZMergeRadius");
      return std::nullopt;
    }

    auto ZMergeRadius_opt = tonumber(L, -1);
    lua_pop(L, 1);

    if (!ZMergeRadius_opt)
    {
      spdlog::get("cads")->error("{{ func = {},  msg = 'ZMergeRadius not a number' }}", __func__);
      return std::nullopt;
    }



    if (lua_getfield(L, index, "XMergeRadius") == LUA_TNIL)
    {
      spdlog::get("cads")->error("{{ func = {},  msg = '{} requires {}' }}", __func__, obj_name, "XMergeRadius");
      return std::nullopt;
    }

    auto XMergeRadius_opt = tointeger(L, -1);
    lua_pop(L, 1);

    if (!XMergeRadius_opt)
    {
      spdlog::get("cads")->error("{{ func = {},  msg = 'XMergeRadius not a integer' }}", __func__);
      return std::nullopt;
    }

    if (lua_getfield(L, index, "MaxClusters") == LUA_TNIL)
    {
      spdlog::get("cads")->error("{{ func = {},  msg = '{} requires {}' }}", __func__, obj_name, "MaxClusters");
      return std::nullopt;
    }

    auto MaxClusters_opt = tointeger(L, -1);
    lua_pop(L, 1);

    if (!MaxClusters_opt)
    {
      spdlog::get("cads")->error("{{ func = {},  msg = 'MaxClusters not a number' }}", __func__);
      return std::nullopt;
    }

    if (*MaxClusters_opt < 0)
    {
      spdlog::get("cads")->error("{{ func = {},  msg = 'MaxClusters less than zero' }}", __func__);
      return std::nullopt;
    }

    return cads::Dbscan{*InClusterRadius_opt, (size_t)*MinPoints_opt, *ZMergeRadius_opt, (size_t)*XMergeRadius_opt, (size_t)*MaxClusters_opt};
  }

  std::optional<cads::Fiducial> tofiducial(lua_State *L, int index)
  {
    const std::string obj_name = "fiducial";

    if (!lua_istable(L, index))
    {
      spdlog::get("cads")->error("{{ func = {},  msg = '{} needs to be a table' }}", __func__, obj_name);
      return std::nullopt;
    }

    auto fields = std::make_tuple("FiducialDepth"s, "FiducialX"s, "FiducialY"s, "FiducialGap"s, "EdgeHeight"s);
    auto numbers = std::apply([=](auto &&...args)
                              { return std::make_tuple(tofieldnumber(L, index, obj_name, args)...); },
                              fields);
    auto invalids = std::apply([=](auto &&...args)
                               { return std::make_tuple(!args...); },
                               numbers);
    auto anyinvalid = std::apply([=](auto &&...args)
                                 { return (false || ... || args); },
                                 invalids);

    if (anyinvalid)
    {
      return std::nullopt;
    }
    else
    {
      auto strip_optional = std::apply([=](auto &&...args)
                                       { return std::make_tuple(*args...); },
                                       numbers);
      return std::apply([=](auto &&...args)
                        { return cads::Fiducial{args...}; },
                        strip_optional);
    }
  }

  std::optional<cads::FiducialOriginDetection> tofiducialoriginconfig(lua_State *L, int index)
  {
    const std::string obj_name = "fiducialoriginconfig";

    if (!lua_istable(L, index))
    {
      spdlog::get("cads")->error("{{ func = {},  msg = '{} needs to be a table' }}", __func__, obj_name);
      return std::nullopt;
    }

    if (lua_getfield(L, index, "BeltLength") == LUA_TNIL)
    {
      spdlog::get("cads")->error("{{ func = {},  msg = '{} requires {}' }}", __func__, obj_name, "BeltLength");
      return std::nullopt;
    }

    auto BeltLength_opt = topair<long long, long long>(L, -1);
    lua_pop(L, 1);

    if (!BeltLength_opt)
    {
      spdlog::get("cads")->error("{{ func = {},  msg = 'BeltLength not a integer' }}", __func__);
      return std::nullopt;
    }

    auto CrossCorrelationThreshold_opt = tofieldnumber(L, index, obj_name, "CrossCorrelationThreshold"s);

    if (!CrossCorrelationThreshold_opt)
    {
      return std::nullopt;
    }

    if (lua_getfield(L, index, "DumpMatch") == LUA_TNIL)
    {
      spdlog::get("cads")->error("{{ func = {},  msg = '{} requires {}' }}", __func__, obj_name, "DumpMatch"s);
      return std::nullopt;
    }

    bool DumpMatch = lua_toboolean(L, -1);
    lua_pop(L, 1);

    if (lua_getfield(L, index, "Fiducial") == LUA_TNIL)
    {
      spdlog::get("cads")->error("{{ func = {},  msg = '{} requires {}' }}", __func__, obj_name, "Fiducial"s);
      return std::nullopt;
    }

    auto Fiducial_opt = tofiducial(L, -1);
    lua_pop(L, 1);

    if (!Fiducial_opt)
    {
      spdlog::get("cads")->error("{{ func = {},  msg = 'BeltLength not a fiducial' }}", __func__);
      return std::nullopt;
    }

    auto length_opt = tofieldnumber(L, index, obj_name, "length"s);

    if (!length_opt)
    {
      return std::nullopt;
    }

    auto avg_y_res_opt = tofieldnumber(L, index, obj_name, "avg_y_res"s);

    if (!avg_y_res_opt)
    {
      return std::nullopt;
    }

    auto width_n_opt = tofieldnumber(L, index, obj_name, "width_n"s);

    if (!width_n_opt)
    {
      return std::nullopt;
    }

    return cads::FiducialOriginDetection{*BeltLength_opt, *CrossCorrelationThreshold_opt, DumpMatch, *Fiducial_opt, *length_opt,*avg_y_res_opt,*width_n_opt};
  }

  std::optional<cads::IIRFilterConfig> toiirfilter(lua_State *L, int index)
  {
    const std::string obj_name = "iirfilter";

    if (!lua_istable(L, index))
    {
      spdlog::get("cads")->error("{{ func = {},  msg = 'iirfilter needs to be a table' }}", __func__);
      return std::nullopt;
    }

    if (lua_getfield(L, index, "Skip") == LUA_TNIL)
    {
      spdlog::get("cads")->error("{{ func = {},  msg = '{} requires {}' }}", __func__, obj_name, "Skip");
      return std::nullopt;
    }

    auto skip_opt = tointeger(L, -1);
    lua_pop(L, 1);

    if (!skip_opt)
    {
      spdlog::get("cads")->error("{{ func = {},  msg = 'Skip not a integer' }}", __func__);
      return std::nullopt;
    }

    if (lua_getfield(L, index, "Delay") == LUA_TNIL)
    {
      spdlog::get("cads")->error("{{ func = {},  msg = '{} requires {}' }}", __func__, obj_name, "Delay");
      return std::nullopt;
    }

    auto delay_opt = tointeger(L, -1);
    lua_pop(L, 1);

    if (!delay_opt)
    {
      spdlog::get("cads")->error("{{ func = {},  msg = 'Delay not a integer' }}", __func__);
      return std::nullopt;
    }

    if (lua_getfield(L, index, "Sos") == LUA_TNIL)
    {
      spdlog::get("cads")->error("{{ func = {},  msg = '{} requires {}' }}", __func__, obj_name, "Sos");
      return std::nullopt;
    }

    auto sos_opt = to2Dnumbervector(L, -1);
    lua_pop(L, 1);

    if (!sos_opt)
    {
      spdlog::get("cads")->error("{{ func = {},  msg = 'Sos not a 2D array of numbers' }}", __func__);
      return std::nullopt;
    }

    if ((*sos_opt).size() != 10)
    {
      spdlog::get("cads")->error("{{ func = {},  msg = 'Sos array requires 10 sub arrays' }}", __func__);
      return std::nullopt;
    }

    for (auto e : *sos_opt)
    {
      if (e.size() != 6)
      {
        spdlog::get("cads")->error("{{ func = {},  msg = 'Sos sub array requires 6 numbers' }}", __func__);
        return std::nullopt;
      }
    }

    return cads::IIRFilterConfig{*skip_opt, *delay_opt, *sos_opt};
  }

  std::optional<cads::RevolutionSensorConfig> torevolutionsensor(lua_State *L, int index)
  {
    const std::string obj_name = "revolutionsensor";

    if (!lua_istable(L, index))
    {
      spdlog::get("cads")->error("{{ func = {},  msg = '{} needs to be a table' }}", __func__, obj_name);
      return std::nullopt;
    }

    if (lua_getfield(L, index, "Source") == LUA_TNIL)
    {
      spdlog::get("cads")->error("{{ func = {},  msg = '{} requires {}' }}", __func__, obj_name, "Source");
      return std::nullopt;
    }

    auto source_opt = tostring(L, -1);
    lua_pop(L, 1);

    if (!source_opt)
    {
      spdlog::get("cads")->error("{{ func = {},  msg = 'Source not a string' }}", __func__);
      return std::nullopt;
    }

    if (lua_getfield(L, index, "TriggerDistance") == LUA_TNIL)
    {
      spdlog::get("cads")->error("{{ func = {},  msg = '{} requires {}' }}", __func__, obj_name, "TriggerDistance");
      return std::nullopt;
    }

    auto trigger_dis_opt = tonumber(L, -1);
    lua_pop(L, 1);

    if (!trigger_dis_opt)
    {
      spdlog::get("cads")->error("{{ func = {},  msg = 'TriggerDistance not a number' }}", __func__);
      return std::nullopt;
    }

    if (lua_getfield(L, index, "Bias") == LUA_TNIL)
    {
      spdlog::get("cads")->error("{{ func = {},  msg = '{} requires {}' }}", __func__, obj_name, "Bias");
      return std::nullopt;
    }

    auto bias_opt = tonumber(L, -1);
    lua_pop(L, 1);

    if (!bias_opt)
    {
      spdlog::get("cads")->error("{{ func = {},  msg = 'Bias not a numbers' }}", __func__);
      return std::nullopt;
    }

    if (lua_getfield(L, index, "Threshold") == LUA_TNIL)
    {
      spdlog::get("cads")->error("{{ func = {},  msg = '{} requires {}' }}", __func__, obj_name, "Threshold");
      return std::nullopt;
    }

    auto theshold_opt = tonumber(L, -1);
    lua_pop(L, 1);

    if (!theshold_opt)
    {
      spdlog::get("cads")->error("{{ func = {},  msg = 'Threshold not a number' }}", __func__);
      return std::nullopt;
    }

    if (lua_getfield(L, index, "Bidirectional") == LUA_TNIL)
    {
      spdlog::get("cads")->error("{{ func = {},  msg = '{} requires {}' }}", __func__, obj_name, "Bidirectional");
      return std::nullopt;
    }

    bool bidirectional = lua_toboolean(L, -1);
    lua_pop(L, 1);

    if (lua_getfield(L, index, "Skip") == LUA_TNIL)
    {
      spdlog::get("cads")->error("{{ func = {},  msg = '{} requires {}' }}", __func__, obj_name, "Skip");
      return std::nullopt;
    }

    auto skip_opt = tointeger(L, -1);
    lua_pop(L, 1);

    if (!skip_opt)
    {
      spdlog::get("cads")->error("{{ func = {},  msg = 'Skip not a number' }}", __func__);
      return std::nullopt;
    }

    cads::RevolutionSensorConfig::Source source;
    if (*source_opt == "raw")
    {
      source = cads::RevolutionSensorConfig::Source::height_raw;
    }
    else if (*source_opt == "length")
    {
      source = cads::RevolutionSensorConfig::Source::length;
    }
    else
    {
      source = cads::RevolutionSensorConfig::Source::height_filtered;
    }

    return cads::RevolutionSensorConfig{source, *trigger_dis_opt, *bias_opt, *theshold_opt, bidirectional, *skip_opt};
  }

  std::optional<cads::MeasureConfig> tomeasureconfig(lua_State *L, int index)
  {
    if (!lua_istable(L, index))
    {
      spdlog::get("cads")->error("{{ func = {},  msg = 'measure config needs to be a table' }}", __func__);
      return std::nullopt;
    }

    if (lua_getfield(L, index, "Enable") == LUA_TNIL)
    {
      spdlog::get("cads")->error("{{ func = {},  msg = 'measure config requires {}' }}", __func__, "Enable");
      return std::nullopt;
    }

    auto Enable = (bool)lua_toboolean(L, -1);
    lua_pop(L, 1);

    return cads::MeasureConfig{Enable};
  }

  std::optional<cads::ProfileConfig> toprofileconfig(lua_State *L, int index)
  {
    if (!lua_istable(L, index))
    {
      spdlog::get("cads")->error("{{ func = {},  msg = 'profile config needs to be a table' }}", __func__);
      return std::nullopt;
    }

    if (lua_getfield(L, index, "Width") == LUA_TNIL)
    {
      spdlog::get("cads")->error("{{ func = {},  msg = 'profile config requires {}' }}", __func__, "Width");
      return std::nullopt;
    }

    auto width_opt = tonumber(L, -1);
    lua_pop(L, 1);

    if (!width_opt)
    {
      spdlog::get("cads")->error("{{ func = {},  msg = 'Width not a number' }}", __func__);
      return std::nullopt;
    }

    if (lua_getfield(L, index, "NaNPercentage") == LUA_TNIL)
    {
      spdlog::get("cads")->error("{{ func = {},  msg = 'profile config requires {}' }}", __func__, "NaNPercentage");
      return std::nullopt;
    }

    auto nanpercentage_opt = tonumber(L, -1);
    lua_pop(L, 1);

    if (!nanpercentage_opt)
    {
      spdlog::get("cads")->error("{{ func = {},  msg = 'NaNPercentage not a number' }}", __func__);
      return std::nullopt;
    }

    if (lua_getfield(L, index, "ClipHeight") == LUA_TNIL)
    {
      spdlog::get("cads")->error("{{ func = {},  msg = 'profile config requires {}' }}", __func__, "ClipHeight");
      return std::nullopt;
    }

    auto clipheight_opt = tonumber(L, -1);
    lua_pop(L, 1);

    if (!clipheight_opt)
    {
      spdlog::get("cads")->error("{{ func = {},  msg = 'ClipHeight not a number' }}", __func__);
      return std::nullopt;
    }

    if (lua_getfield(L, index, "PulleyEstimatorInit") == LUA_TNIL)
    {
      spdlog::get("cads")->error("{{ func = {},  msg = 'profile config requires {}' }}", __func__, "PulleyEstimatorInit");
      return std::nullopt;
    }

    auto PulleyEstimatorInit_opt = tonumber(L, -1);
    lua_pop(L, 1);

    if (!PulleyEstimatorInit_opt)
    {
      spdlog::get("cads")->error("{{ func = {},  msg = 'PulleyEstimatorInit not a number' }}", __func__);
      return std::nullopt;
    }

    if (lua_getfield(L, index, "ClampToZeroHeight") == LUA_TNIL)
    {
      spdlog::get("cads")->error("{{ func = {},  msg = 'profile config requires {}' }}", __func__, "ClampToZeroHeight");
      return std::nullopt;
    }

    auto ClampToZeroHeight_opt = tonumber(L, -1);
    lua_pop(L, 1);

    if (!ClampToZeroHeight_opt)
    {
      spdlog::get("cads")->error("{{ func = {},  msg = 'ClampToZeroHeight not a number' }}", __func__);
      return std::nullopt;
    }

    if (lua_getfield(L, index, "TypicalSpeed") == LUA_TNIL)
    {
      spdlog::get("cads")->error("{{ func = {},  msg = 'profile config requires {}' }}", __func__, "TypicalSpeed");
      return std::nullopt;
    }

    auto TypicalSpeed_opt = tonumber(L, -1);
    lua_pop(L, 1);

    if (!TypicalSpeed_opt)
    {
      spdlog::get("cads")->error("{{ func = {},  msg = 'TypicalSpeed not a number' }}", __func__);
      return std::nullopt;
    }

    if (lua_getfield(L, index, "PulleyCircumference") == LUA_TNIL)
    {
      spdlog::get("cads")->error("{{ func = {},  msg = 'profile config requires {}' }}", __func__, "PulleyCircumference");
      return std::nullopt;
    }

    auto PulleyCircumference_opt = tonumber(L, -1);
    lua_pop(L, 1);

    if (!PulleyCircumference_opt)
    {
      spdlog::get("cads")->error("{{ func = {},  msg = 'PulleyCircumference not a number' }}", __func__);
      return std::nullopt;
    }

    if (lua_getfield(L, index, "WidthN") == LUA_TNIL)
    {
      spdlog::get("cads")->error("{{ func = {},  msg = 'profile config requires {}' }}", __func__, "WidthN");
      return std::nullopt;
    }

    auto WidthN_opt = tointeger(L, -1);
    lua_pop(L, 1);

    if (!WidthN_opt)
    {
      spdlog::get("cads")->error("{{ func = {},  msg = 'WidthN not a integer' }}", __func__);
      return std::nullopt;
    }

    if (lua_getfield(L, index, "IIRFilter") == LUA_TNIL)
    {
      spdlog::get("cads")->error("{{ func = {},  msg = 'profile config requires {}' }}", __func__, "IIRFilter");
      return std::nullopt;
    }

    auto iirfilter_opt = toiirfilter(L, -1);
    lua_pop(L, 1);

    if (!iirfilter_opt)
    {
      spdlog::get("cads")->error("{{ func = {},  msg = 'IIRFilter not a IIRFilterConfig' }}", __func__);
      return std::nullopt;
    }


    if (lua_getfield(L, index, "RevolutionSensor") == LUA_TNIL)
    {
      spdlog::get("cads")->error("{{ func = {},  msg = 'profile config requires {}' }}", __func__, "RevolutionSensor");
      return std::nullopt;
    }

    auto revolution_sensor_opt = torevolutionsensor(L, -1);
    lua_pop(L, 1);

    if (!revolution_sensor_opt)
    {
      spdlog::get("cads")->error("{{ func = {},  msg = 'RevolutionSensor not a RevolutionSensorConfig' }}", __func__);
      return std::nullopt;
    }


    if (lua_getfield(L, index, "Measures") == LUA_TNIL)
    {
      spdlog::get("cads")->error("{{ func = {},  msg = 'profile config requires {}' }}", __func__, "Measures");
      return std::nullopt;
    }

    auto Measures_opt = tomeasureconfig(L, -1);
    lua_pop(L, 1);

    if (!Measures_opt)
    {
      spdlog::get("cads")->error("{{ func = {},  msg = 'Measures not a Measures' }}", __func__);
      return std::nullopt;
    }

    return cads::ProfileConfig{*width_opt, *nanpercentage_opt, *clipheight_opt,*PulleyEstimatorInit_opt,  *ClampToZeroHeight_opt, *TypicalSpeed_opt,*PulleyCircumference_opt,*WidthN_opt,*iirfilter_opt, *revolution_sensor_opt,*Measures_opt};
  }

  std::optional<cads::SqliteGocatorConfig> tosqlitegocatorconfig(lua_State *L, int index)
  {
    if (!lua_istable(L, index))
    {
      spdlog::get("cads")->error("{{ func = {},  msg = 'sqlite gocator needs to be a lua table' }}", __func__);
      return std::nullopt;
    }

    if (lua_getfield(L, index, "Range") == LUA_TNIL)
    {
      spdlog::get("cads")->error("{{ func = {},  msg = 'sqlite gocator Config requires {}' }}", __func__, "Range");
      return std::nullopt;
    }

    auto range_opt = topair<long long, long long>(L, -1);
    lua_pop(L, 1);

    if (!range_opt)
    {
      spdlog::get("cads")->error("{{ func = {},  msg = 'range not a tuple pair with integers' }}", __func__);
      return std::nullopt;
    }

    if (lua_getfield(L, index, "Fps") == LUA_TNIL)
    {
      spdlog::get("cads")->error("{{ func = {},  msg = 'sqlite gocator Config requires {}' }}", __func__, "Fps");
      return std::nullopt;
    }

    auto fps_opt = tonumber(L, -1);
    lua_pop(L, 1);

    if (!fps_opt)
    {
      spdlog::get("cads")->error("{{ func = {},  msg = 'fps not a number' }}", __func__);
      return std::nullopt;
    }

    if (*fps_opt < 1)
    {
      spdlog::get("cads")->error("{{ func = {},  msg = 'fps less than 1' }}", __func__);
      return std::nullopt;
    }

    if (lua_getfield(L, index, "Forever") == LUA_TNIL)
    {
      spdlog::get("cads")->error("{{ func = {},  msg = 'sqlite gocator Config requires {}' }}", __func__, "Forever");
      return std::nullopt;
    }

    bool forever = lua_toboolean(L, -1);
    lua_pop(L, 1);

    if (lua_getfield(L, index, "Source") == LUA_TNIL)
    {
      spdlog::get("cads")->error("{{ func = {},  msg = 'sqlite gocator Config requires {}' }}", __func__, "Source");
      return std::nullopt;
    }

    auto source_opt = tostring(L, -1);
    lua_pop(L, 1);

    if (!source_opt)
    {
      spdlog::get("cads")->error("{{ func = {},  msg = 'source not a string' }}", __func__);
      return std::nullopt;
    }

    if (lua_getfield(L, index, "TypicalSpeed") == LUA_TNIL)
    {
      spdlog::get("cads")->error("{{ func = {},  msg = 'sqlite gocator Config requires {}' }}", __func__, "TypicalSpeed");
      return std::nullopt;
    }
    auto typical_speed_opt = tonumber(L, -1);
    lua_pop(L, 1);

    if (!typical_speed_opt)
    {
      spdlog::get("cads")->error("{{ func = {},  msg = 'source not a string' }}", __func__);
      return std::nullopt;
    }

    if (lua_getfield(L, index, "Sleep") == LUA_TNIL)
    {
      spdlog::get("cads")->error("{{ func = {},  msg = 'sqlite gocator Config requires {}' }}", __func__, "Sleep");
      return std::nullopt;
    }

    bool Sleep = lua_toboolean(L, -1);
    lua_pop(L, 1);

    return cads::SqliteGocatorConfig{*range_opt, *fps_opt, forever, *source_opt, *typical_speed_opt, Sleep};
  }

  std::optional<cads::GocatorConfig> togocatorconfig(lua_State *L, int index)
  {
    const std::string obj_name = "gocatorconfig";

    if (!lua_istable(L, index))
    {
      spdlog::get("cads")->error("{{ func = {},  msg = '{} needs to be a lua table' }}", __func__, obj_name);
      return std::nullopt;
    }

    if (lua_getfield(L, index, "Trim") == LUA_TNIL)
    {
      spdlog::get("cads")->error("{{ func = {},  msg = '{} requires {}' }}", __func__, obj_name, "Trim");
      return std::nullopt;
    }

    lua_getfield(L, index, "Trim");
    bool trim = lua_toboolean(L, -1);
    lua_pop(L, 1);

    if (lua_getfield(L, index, "TypicalResolution") == LUA_TNIL)
    {
      spdlog::get("cads")->error("{{ func = {},  msg = '{} requires {}' }}", __func__, obj_name, "TypicalResolution");
      return std::nullopt;
    }

    lua_getfield(L, index, "TypicalResolution");
    auto typical_resolution_opt = tonumber(L, -1);
    lua_pop(L, 1);

    if (!typical_resolution_opt)
    {
      spdlog::get("cads")->error("{{ func = {},  msg = 'TypicalResolution not a number' }}", __func__);
      return std::nullopt;
    }

    return cads::GocatorConfig{trim, *typical_resolution_opt};
  }

  std::optional<cads::DynamicProcessingConfig> todynamicprocessingconfig(lua_State *L, int index)
  {
    const std::string obj_name = "dynamicprocessingconfig";

    if (!lua_istable(L, index))
    {
      spdlog::get("cads")->error("{{ func = {},  msg = '{} needs to be a lua table' }}", __func__, obj_name);
      return std::nullopt;
    }

    if (lua_getfield(L, index, "WidthN") == LUA_TNIL)
    {
      spdlog::get("cads")->error("{{ func = {},  msg = '{} requires {}' }}", __func__, obj_name, "WidthN");
      return std::nullopt;
    }

    auto WidthN_opt = tointeger(L, -1);
    lua_pop(L, 1);

    if (!WidthN_opt)
    {
      spdlog::get("cads")->error("{{ func = {},  msg = 'WidthN not a integer' }}", __func__);
      return std::nullopt;
    }

    if (lua_getfield(L, index, "WindowSize") == LUA_TNIL)
    {
      spdlog::get("cads")->error("{{ func = {},  msg = '{} requires {}' }}", __func__, obj_name, "WindowSize");
      return std::nullopt;
    }

    auto WindowSize_opt = tointeger(L, -1);
    lua_pop(L, 1);

    if (!WindowSize_opt)
    {
      spdlog::get("cads")->error("{{ func = {},  msg = 'WindowSize not a integer' }}", __func__);
      return std::nullopt;
    }

    if (lua_getfield(L, index, "InitValue") == LUA_TNIL)
    {
      spdlog::get("cads")->error("{{ func = {},  msg = '{} requires {}' }}", __func__, obj_name, "InitValue");
      return std::nullopt;
    }

    auto InitValue_opt = tonumber(L, -1);
    lua_pop(L, 1);

    if (!InitValue_opt)
    {
      spdlog::get("cads")->error("{{ func = {},  msg = 'InitValue not a number' }}", __func__);
      return std::nullopt;
    }

    if (lua_getfield(L, index, "LuaCode") == LUA_TNIL)
    {
      spdlog::get("cads")->error("{{ func = {},  msg = '{} requires {}' }}", __func__, obj_name, "LuaCode");
      return std::nullopt;
    }

    auto LuaCode_opt = tostring(L, -1);
    lua_pop(L, 1);

    if (!LuaCode_opt)
    {
      spdlog::get("cads")->error("{{ func = {},  msg = 'LuaCode not a string' }}", __func__);
      return std::nullopt;
    }

    if (lua_getfield(L, index, "Entry") == LUA_TNIL)
    {
      spdlog::get("cads")->error("{{ func = {},  msg = '{} requires {}' }}", __func__, obj_name, "Entry");
      return std::nullopt;
    }

    auto Entry_opt = tostring(L, -1);
    lua_pop(L, 1);

    if (!Entry_opt)
    {
      spdlog::get("cads")->error("{{ func = {},  msg = 'Entry not a string' }}", __func__);
      return std::nullopt;
    }

    return cads::DynamicProcessingConfig{*WidthN_opt, *WindowSize_opt, *InitValue_opt, *LuaCode_opt, *Entry_opt};
  }

  void pushmetric(lua_State *L, cads::Measure::MeasureMsg m)
  {
    auto [sub, quality, time, value] = m;

    lua_newtable(L);
    lua_pushnumber(L, 1);
    lua_pushstring(L, sub.c_str());
    lua_settable(L, -3);

    lua_pushnumber(L, 2);
    lua_pushnumber(L, quality);
    lua_settable(L, -3);

    lua_pushnumber(L, 3);
    auto tp = std::chrono::duration<double>(get<2>(m).time_since_epoch());
    lua_pushnumber(L, tp.count());
    lua_settable(L, -3);

    switch (value.index())
    {
    case 0:
      lua_pushnumber(L, 4);
      lua_pushnumber(L, get<double>(value));
      lua_settable(L, -3);
      break;
    case 1:
      lua_pushnumber(L, 4);
      lua_pushstring(L, get<std::string>(value).c_str());
      lua_settable(L, -3);
      break;
    case 2:
      lua_pushnumber(L, 4);
      lua_pushlightuserdata(L, &value);
      lua_pushcclosure(L, execute_func2<double>, 1);
      lua_settable(L, -3);
      break;
    case 3:
      lua_pushnumber(L, 4);
      lua_pushlightuserdata(L, &value);
      lua_pushcclosure(L, execute_func2<std::string>, 1);
      lua_settable(L, -3);
      break;
    case 4:
    {
      auto [v, location] = get<std::tuple<double, double>>(value);
      lua_pushnumber(L, 4);
      lua_pushnumber(L, v);
      lua_settable(L, -3);
      lua_pushnumber(L, 5);
      lua_pushnumber(L, location);
      lua_settable(L, -3);
      break;
    }
    default:
      break;
    }
  }

  void pushcaasmsg(lua_State *L, cads::CaasMsg m)
  {
    auto [sub, data] = m;

    lua_newtable(L);
    lua_pushnumber(L, 1);
    lua_pushlstring(L, sub.c_str(), sub.size());
    lua_settable(L, -3);

    lua_pushnumber(L, 2);
    lua_pushlstring(L, data.c_str(), data.size());
    lua_settable(L, -3);
  }

  std::optional<cads::AnomalyDetection> mk_anomaly(lua_State *L, int index)
  {

    cads::AnomalyDetection obj;
    int isnum = 0; // false

    if (lua_istable(L, index))
    {
      if (lua_getfield(L, index, "WindowSize") == LUA_TNUMBER)
      {
        obj.WindowSize = lua_tonumberx(L, -1, &isnum);
        lua_pop(L, 1);
        if (isnum == 0)
          goto ERR;
      }
      else
        goto ERR;

      if (lua_getfield(L, index, "BeltPartitionSize") == LUA_TNUMBER)
      {
        obj.BeltPartitionSize = lua_tonumberx(L, -1, &isnum);
        lua_pop(L, 1);
        if (isnum == 0)
          goto ERR;
      }
      else
        goto ERR;

      if (lua_getfield(L, index, "BeltSize") == LUA_TNUMBER)
      {
        obj.BeltSize = lua_tonumberx(L, -1, &isnum);
        lua_pop(L, 1);
        if (isnum == 0)
          goto ERR;
      }
      else
        goto ERR;

      if (lua_getfield(L, index, "MinPosition") == LUA_TNUMBER)
      {
        obj.MinPosition = lua_tonumberx(L, -1, &isnum);
        lua_pop(L, 1);
        if (isnum == 0)
          goto ERR;
      }
      else
        goto ERR;

      if (lua_getfield(L, index, "MaxPosition") == LUA_TNUMBER)
      {
        obj.MaxPosition = lua_tonumberx(L, -1, &isnum);
        lua_pop(L, 1);
        if (isnum == 0)
          goto ERR;
      }
      else
        goto ERR;

      if (lua_getfield(L, index, "ConveyorName") == LUA_TSTRING)
      {
        obj.ConveyorName = lua_tostring(L, -1);
        lua_pop(L, 1);
      }
      else
        goto ERR;

      return obj;
    }

  ERR:
    return std::nullopt;
  }

  int Io_gc(lua_State *L)
  {
    auto q = static_cast<cads::Io<cads::msg> *>(lua_touserdata(L, 1));
    q->~Io();
    return 0;
  }

  int thread_gc(lua_State *L)
  {
    auto t = static_cast<std::thread *>(lua_touserdata(L, 1));
    if (t->joinable())
      t->join();
    t->~thread();
    return 0;
  }

  int sleep_ms(lua_State *L)
  {
    auto ms = lua_tointeger(L, 1);
    std::this_thread::sleep_for(std::chrono::milliseconds(ms));
    lua_pushboolean(L, 1);
    return 1;
  }

  int join_threads(lua_State *L)
  {
    int array_length = lua_rawlen(L, -1);

    for (int i = 1; i <= array_length; i++)
    {
      lua_rawgeti(L, -1, i);
      if (lua_isuserdata(L, -1))
      {
        auto v = lua_touserdata(L, -1);
        auto t = static_cast<std::thread *>(v);
        t->join();
      }
      else
      {
        spdlog::get("cads")->error("{{ func = {},  msg = 'Array contains non userdata' }}", __func__);
      }
      lua_pop(L, 1);
    }

    lua_pushnumber(L, 3);
    return 1;
  }

  int partitionProfile(lua_State *L)
  {
    using namespace std::placeholders;
    auto Dbscan_opt = todbscan(L, 1);

    if (!Dbscan_opt)
    {
      spdlog::get("cads")->error("{{ func = {},  msg = 'First argument not a dbscan' }}", __func__);
      lua_pushnil(L);
      lua_pushboolean(L, 1);
      return 2;
    }

    auto out = static_cast<cads::Io<cads::msg> *>(lua_touserdata(L, 2));
    std::function<cads::msg(cads::msg)> fn(std::bind(cads::partition_profile, _1, *Dbscan_opt));

    new (lua_newuserdata(L, sizeof(cads::AdaptFn<cads::msg>))) cads::AdaptFn<cads::msg>(TransformMsg(fn, out));

    lua_createtable(L, 0, 1);
    lua_pushcfunction(L, Io_gc);
    lua_setfield(L, -2, "__gc");
    lua_setmetatable(L, -2);
    return 1;
  }

  int alignProfile(lua_State *L)
  {
    auto i = tointeger(L,-2);
    if(i) {
      auto out = static_cast<cads::Io<cads::msg> *>(lua_touserdata(L, -1));
      std::function<cads::msg(cads::msg)> fn(cads::mk_align_profile(*i));

      new (lua_newuserdata(L, sizeof(cads::AdaptFn<cads::msg>))) cads::AdaptFn<cads::msg>(TransformMsg(fn, out));

      lua_createtable(L, 0, 1);
      lua_pushcfunction(L, Io_gc);
      lua_setfield(L, -2, "__gc");
      lua_setmetatable(L, -2);
    }else {
      lua_pushnil(L);
    }
    return 1;

  }

  int prsToScan(lua_State *L)
  {
    auto out = static_cast<cads::Io<cads::msg> *>(lua_touserdata(L, -1));
    std::function<cads::msg(cads::msg)> fn(cads::prs_to_scan);

    new (lua_newuserdata(L, sizeof(cads::AdaptFn<cads::msg>))) cads::AdaptFn<cads::msg>(TransformMsg(fn, out));

    lua_createtable(L, 0, 1);
    lua_pushcfunction(L, Io_gc);
    lua_setfield(L, -2, "__gc");
    lua_setmetatable(L, -2);
    return 1;
  }

  int profilePartionToScan(lua_State *L)
  {
    auto out = static_cast<cads::Io<cads::msg> *>(lua_touserdata(L, -1));
    std::function<cads::msg(cads::msg)> fn(cads::profilePartion_to_scan);

    new (lua_newuserdata(L, sizeof(cads::AdaptFn<cads::msg>))) cads::AdaptFn<cads::msg>(TransformMsg(fn, out));

    lua_createtable(L, 0, 1);
    lua_pushcfunction(L, Io_gc);
    lua_setfield(L, -2, "__gc");
    lua_setmetatable(L, -2);
    return 1;
  }

  int extractPartition(lua_State *L)
  {
    auto out = static_cast<cads::Io<cads::z_type> *>(lua_touserdata(L, -1));
    auto i_opt = tointeger(L,-2);

    if(!i_opt) {
      return 0;
    }
    auto i = (int)*i_opt;

    if(i >= (int)cads::ProfileSection::End || i < 0) {
      spdlog::get("cads")->error("{{ func = {},  msg = 'Integer cannot be cast to a ProfileSection'}}", __func__);
      return 0;
    }
    
    double x_res = 1.0;
    std::function<cads::z_type(cads::msg)> fn = [x_res,i](cads::msg m) mutable
    { 
      if(std::get<0>(m) == cads::msgid::profile_partitioned) {
        return extract_partition(std::get<cads::ProfilePartitioned>(std::get<1>(m)),static_cast<cads::ProfileSection>(i)); 
      }else if(std::get<0>(m) == cads::msgid::gocator_properties){
        x_res = std::get<cads::GocatorProperties>(std::get<1>(m)).xResolution;
        return cads::z_type();
      }else {
        return cads::z_type();
      }
    };
    
    new (lua_newuserdata(L, sizeof(cads::AdaptFn<cads::msg>))) cads::AdaptFn<cads::msg>(TransformMsg(fn, out));

    lua_createtable(L, 0, 1);
    lua_pushcfunction(L, Io_gc);
    lua_setfield(L, -2, "__gc");
    lua_setmetatable(L, -2);
    return 1;
  }

  int fileCSV(lua_State *L)
  {
    auto filename = tostring(L, 1);
    auto i_opt = tointeger(L, 2);
    auto x_min = tonumber(L, 3);

    auto i = (int)*i_opt;

    if(i >= (int)cads::ProfileSection::End || i < 0) {
      spdlog::get("cads")->error("{{ func = {},  msg = 'Integer cannot be cast to a ProfileSection'}}", __func__);
      return 0;
    }

    using user_type = cads::Adapt<cads::msg, decltype(cads::file_csv_coro2(*filename,static_cast<cads::ProfileSection>(i),*x_min))>;

    new (lua_newuserdata(L, sizeof(user_type))) user_type(cads::file_csv_coro2(*filename,static_cast<cads::ProfileSection>(i),*x_min));

    lua_createtable(L, 0, 1);
    lua_pushcfunction(L, Io_gc);
    lua_setfield(L, -2, "__gc");
    lua_setmetatable(L, -2);
    return 1;
  }

  int filterMsgs(lua_State *L)
  {
    auto out = static_cast<cads::Io<cads::msg> *>(lua_touserdata(L, -1));
    auto mid = lua_tointeger(L, -2);
    auto id = cads::i_msgid(mid);

    new (lua_newuserdata(L, sizeof(cads::AdaptFn<cads::msg>))) cads::AdaptFn<cads::msg>(FilterMsg(id, out));

    lua_createtable(L, 0, 1);
    lua_pushcfunction(L, Io_gc);
    lua_setfield(L, -2, "__gc");
    lua_setmetatable(L, -2);
    return 1;
  }

  int teeMsg(lua_State *L)
  {
    auto nil = lua_isnil(L,-2) + 2*lua_isnil(L,-1);
    
    if(nil)
    {
      spdlog::get("cads")->error("{{ func = {},  msg = 'Argument is nil {}. The value is a bitmask.'}}", __func__,nil);
      return 0;
    }

    auto out0 = static_cast<cads::Io<cads::msg> *>(lua_touserdata(L, -2));
    auto out1 = static_cast<cads::Io<cads::msg> *>(lua_touserdata(L, -1));

    std::function<bool(cads::msg)> fn = [out0, out1](cads::msg m)
    { (out0 ? out0->enqueue(m) : true); return (out1 ? out1->enqueue(m) : true); };

    new (lua_newuserdata(L, sizeof(cads::AdaptFn<cads::msg>))) cads::AdaptFn<cads::msg>(std::move(fn));

    lua_createtable(L, 0, 1);
    lua_pushcfunction(L, Io_gc);
    lua_setfield(L, -2, "__gc");
    lua_setmetatable(L, -2);
    return 1;
  }

  int BlockingReaderWriterQueue(lua_State *L)
  {
    new (lua_newuserdata(L, sizeof(cads::Adapt<cads::msg, moodycamel::BlockingReaderWriterQueue<cads::msg>>))) cads::Adapt<cads::msg, moodycamel::BlockingReaderWriterQueue<cads::msg>>(moodycamel::BlockingReaderWriterQueue<cads::msg>());

    lua_createtable(L, 0, 1);
    lua_pushcfunction(L, Io_gc);
    lua_setfield(L, -2, "__gc");
    lua_setmetatable(L, -2);

    return 1;
  }

  int wait_for(lua_State *L)
  {
    auto io = static_cast<cads::Io<cads::msg> *>(lua_touserdata(L, 1));
    auto s = lua_tointeger(L, 2);

    cads::msg m;
    auto have_value = io->wait_dequeue_timed(m, std::chrono::seconds(s));
    lua_pushboolean(L, have_value);

    if (!have_value)
    {
      return 1;
    }

    auto mid = std::get<0>(m);
    lua_pushinteger(L, mid);

    if (mid == cads::msgid::measure)
    {
      pushmetric(L, std::get<cads::Measure::MeasureMsg>(std::get<1>(m)));
      return 3;
    }
    else if (mid == cads::msgid::caas_msg)
    {
      pushcaasmsg(L, std::get<cads::CaasMsg>(std::get<1>(m)));
      return 3;
    }
    else if (mid == cads::msgid::error)
    {
      auto errmsg = std::get<std::shared_ptr<cads::errors::Err>>(std::get<1>(m))->str();
      lua_pushlstring(L, errmsg.c_str(), errmsg.size());
      return 3;
    }

    return 2;
  }

  auto thread2_with_catch(std::function<void(cads::Io<cads::msg> &, cads::Io<cads::msg> &)> fn)
  {
    return [=](cads::Io<cads::msg> &input, cads::Io<cads::msg> &output)
    {
      try
      {
        fn(input, output);
      }
      catch (std::exception &ex)
      {
        output.enqueue({cads::msgid::error, ex.what()});
      }
    };
  }

  int mk_thread2(lua_State *L, std::function<void(cads::Io<cads::msg> &, cads::Io<cads::msg> &)> fn)
  {
    auto in = static_cast<cads::Io<cads::msg> *>(lua_touserdata(L, -2));
    auto out = static_cast<cads::Io<cads::msg> *>(lua_touserdata(L, -1));

    new (lua_newuserdata(L, sizeof(std::thread))) std::thread(thread2_with_catch(fn), std::ref(*in), std::ref(*out));
    lua_createtable(L, 0, 1);
    lua_pushcfunction(L, thread_gc);
    lua_setfield(L, -2, "__gc");
    lua_setmetatable(L, -2);
    return 1;
  }

  int gocator_start(lua_State *L)
  {
    auto gocator = static_cast<std::unique_ptr<cads::GocatorReaderBase> *>(lua_touserdata(L, -2));
    auto fps = lua_tonumber(L, -1);
    auto err = (*gocator)->Start(fps);

    lua_pushboolean(L, err);

    return 1;
  }

  int gocator_stop(lua_State *L)
  {
    auto gocator = static_cast<std::unique_ptr<cads::GocatorReaderBase> *>(lua_touserdata(L, -2));
    auto signal_finished = lua_toboolean(L, -1);
    (*gocator)->Stop(signal_finished);

    return 0;
  }

  int gocator_align(lua_State *L)
  {
    auto gocator = static_cast<std::unique_ptr<cads::GocatorReaderBase> *>(lua_touserdata(L, -1));
    auto err = (*gocator)->Align();

    lua_pushboolean(L, err);

    return 1;
  }

  int gocator_setfov(lua_State *L)
  {
    auto gocator = static_cast<std::unique_ptr<cads::GocatorReaderBase> *>(lua_touserdata(L, -2));
    auto len = lua_tonumber(L, -1);
    auto err = (*gocator)->SetFoV(len);

    lua_pushboolean(L, err);

    return 1;
  }

  int gocator_reset_align(lua_State *L)
  {
    auto gocator = static_cast<std::unique_ptr<cads::GocatorReaderBase> *>(lua_touserdata(L, -1));
    auto err = (*gocator)->ResetAlign();

    lua_pushboolean(L, err);

    return 1;
  }

  int gocator_gc(lua_State *L)
  {
    auto gocator = static_cast<std::unique_ptr<cads::GocatorReaderBase> *>(lua_touserdata(L, -1));
    gocator->~unique_ptr<cads::GocatorReaderBase>();

    return 0;
  }

  void pushgocatormeta(lua_State *L)
  {
    lua_createtable(L, 0, 1);
    lua_pushcfunction(L, gocator_gc);
    lua_setfield(L, -2, "__gc");
    lua_createtable(L, 0, 2);
    lua_pushcfunction(L, gocator_start);
    lua_setfield(L, -2, "Start");
    lua_pushcfunction(L, gocator_stop);
    lua_setfield(L, -2, "Stop");
    lua_pushcfunction(L, gocator_align);
    lua_setfield(L, -2, "Align");
    lua_pushcfunction(L, gocator_setfov);
    lua_setfield(L, -2, "SetFoV");
    lua_pushcfunction(L, gocator_reset_align);
    lua_setfield(L, -2, "ResetAlign");

    lua_setfield(L, -2, "__index");
  }

  int sqlitegocator(lua_State *L)
  {
    auto sqlite_gocator_config_opt = tosqlitegocatorconfig(L, 1);

    if (!sqlite_gocator_config_opt)
    {
      lua_pushnil(L);
      return 1;
    }

    auto q = static_cast<cads::Io<cads::msg> *>(lua_touserdata(L, 2));
    auto p = new (lua_newuserdata(L, sizeof(std::unique_ptr<cads::GocatorReaderBase>))) std::unique_ptr<cads::SqliteGocatorReader>;

    try
    {
      *p = std::make_unique<cads::SqliteGocatorReader>(*sqlite_gocator_config_opt, *q);
      pushgocatormeta(L);
      lua_setmetatable(L, -2);

      return 1;
    }
    catch (std::exception &ex)
    {
      spdlog::get("cads")->error(R"({{func = '{}', msg = '{}'}})", __func__, ex.what());
    }

    return 0;
  }

  int gocator(lua_State *L)
  {
    auto gocator_config_opt = togocatorconfig(L, 1);

    if (!gocator_config_opt)
    {
      lua_pushnil(L);
      return 1;
    }

    auto q = static_cast<cads::Io<cads::msg> *>(lua_touserdata(L, 2));
    auto p = new (lua_newuserdata(L, sizeof(std::unique_ptr<cads::GocatorReaderBase>))) std::unique_ptr<cads::GocatorReader>;

    try
    {
      *p = std::make_unique<cads::GocatorReader>(*gocator_config_opt, *q);
      pushgocatormeta(L);
      lua_setmetatable(L, -2);

      return 1;
    }
    catch (std::exception &ex)
    {
      spdlog::get("cads")->error(R"({{func = '{}', msg = '{}'}})", __func__, ex.what());
    }

    return 0;
  }

  int anomaly_detection_thread(lua_State *L)
  {
    using namespace std::placeholders;
    auto anomaly = mk_anomaly(L, 1);

    if (anomaly)
    {
      auto bound = std::bind(cads::splice_detection_thread, *anomaly, _1, _2);
      mk_thread2(L, bound);
      lua_pushboolean(L, 0);
    }
    else
    {
      lua_pushnil(L);
      lua_pushboolean(L, 1);
    }

    return 2;
  }

  int fiducial_origin_thread(lua_State *L)
  {
    using namespace std::placeholders;

    auto config = tofiducialoriginconfig(L, 1);
    auto bound = std::bind(cads::fiducial_origin_thread, *config, _1, _2);
    return mk_thread2(L, bound);
  }

  int dynamic_processing_thread(lua_State *L)
  {
    using namespace std::placeholders;

    auto config = todynamicprocessingconfig(L, 1);
    auto bound = std::bind(cads::dynamic_processing_thread, *config, _1, _2);

    return mk_thread2(L, bound);
  }

  int save_send_thread(lua_State *L)
  {
    using namespace std::placeholders;
    const auto expected_arg_cnt = 3;

    auto arg_cnt = lua_gettop(L);

    if (arg_cnt > expected_arg_cnt)
    {
      spdlog::get("cads")->error("{{ func = {},  msg = 'Requires {} arguemtns not {}'}}", __func__,expected_arg_cnt,arg_cnt);
    }

    auto config = toscanstorageconfig(L, 1);
    auto bound = std::bind(cads::save_send_thread, *config, _1, _2);
    
    return mk_thread2(L, bound);
  }

  int loop_beltlength_thread(lua_State *L)
  {
    using namespace std::placeholders;

    auto length = tonumber(L, 1);
    auto bound = std::bind(cads::loop_beltlength_thread, *length, _1, _2);
    return mk_thread2(L, bound);
  }

  int process_profile(lua_State *L)
  {
    using namespace std::placeholders;
    auto profile_config = toprofileconfig(L, 1);
    auto bound = std::bind(cads::process_profile, *profile_config, _1, _2);
    return mk_thread2(L, bound);
  }

  int profile_pulley_translate(lua_State *L)
  {
    using namespace std::placeholders;
    auto profile_config = toprofileconfig(L, 1);
    auto bound = std::bind(cads::profile_pulley_translate, *profile_config, _1, _2);
    return mk_thread2(L, bound);
  }

  int process_identity(lua_State *L)
  {
    return mk_thread2(L, cads::process_identity);
  }

  int encoder_distance_estimation(lua_State *L)
  {

    auto next = static_cast<cads::Io<cads::msg> *>(lua_touserdata(L, 1));
    double stride = lua_tonumber(L, 2);

    using user_type = cads::Adapt<cads::msg, decltype(cads::encoder_distance_estimation(std::ref(*next), stride))>;

    new (lua_newuserdata(L, sizeof(user_type))) user_type(cads::encoder_distance_estimation(std::ref(*next), stride));

    lua_createtable(L, 0, 1);
    lua_pushcfunction(L, Io_gc);
    lua_setfield(L, -2, "__gc");
    lua_setmetatable(L, -2);
    return 1;
  }

  int voidMsg(lua_State *L)
  {

    using user_type = cads::Adapt<cads::msg, decltype(cads::void_msg())>;

    new (lua_newuserdata(L, sizeof(user_type))) user_type(cads::void_msg());

    lua_createtable(L, 0, 1);
    lua_pushcfunction(L, Io_gc);
    lua_setfield(L, -2, "__gc");
    lua_setmetatable(L, -2);
    return 1;
  }

  int profile_decimation(lua_State *L)
  {

    auto widthn = tointeger(L, 1);
    double modulo = lua_tointeger(L, 2);
    auto next = static_cast<cads::Io<cads::msg> *>(lua_touserdata(L, 3));

    using user_type = cads::Adapt<cads::msg, decltype(cads::profile_decimation_coro(*widthn, modulo, std::ref(*next)))>;

    new (lua_newuserdata(L, sizeof(user_type))) user_type(cads::profile_decimation_coro(*widthn, modulo, std::ref(*next)));

    lua_createtable(L, 0, 1);
    lua_pushcfunction(L, Io_gc);
    lua_setfield(L, -2, "__gc");
    lua_setmetatable(L, -2);
    return 1;
  }

  int get_serial(lua_State *L)
  {
    lua_pushinteger(L, cads::constants_device.Serial);
    return 1;
  }

  int cadsLog(lua_State *L)
  {
    size_t strlen = 0;
    auto str = lua_tolstring(L, 1, &strlen);
    std::string sub(str, strlen);
    spdlog::get("cads")->error(sub);
    return 0;
  }

}

namespace cads
{
  namespace lua
  {

    Lua init(Lua UL, long long serial, std::string lua_code)
    {
      auto L = UL.get();

      lua_pushinteger(L, serial);
      lua_setglobal(L, "DeviceSerial");

      lua_pushstring(L, lua_code.c_str());
      lua_setglobal(L, "LuaCodeSelfRef");

      lua_pushcfunction(L, ::BlockingReaderWriterQueue);
      lua_setglobal(L, "BlockingReaderWriterQueue");

      lua_pushcfunction(L, ::anomaly_detection_thread);
      lua_setglobal(L, "anomaly_detection_thread");

      lua_pushcfunction(L, ::save_send_thread);
      lua_setglobal(L, "save_send_thread");

      lua_pushcfunction(L, ::fiducial_origin_thread);
      lua_setglobal(L, "fiducial_origin_thread");

      lua_pushcfunction(L, ::loop_beltlength_thread);
      lua_setglobal(L, "loop_beltlength_thread");

      lua_pushcfunction(L, ::dynamic_processing_thread);
      lua_setglobal(L, "dynamic_processing_thread");

      lua_pushcfunction(L, ::wait_for);
      lua_setglobal(L, "wait_for");

      lua_pushcfunction(L, ::join_threads);
      lua_setglobal(L, "join_threads");

      lua_pushcfunction(L, ::gocator);
      lua_setglobal(L, "gocator");

      lua_pushcfunction(L, ::sqlitegocator);
      lua_setglobal(L, "sqlitegocator");

      lua_pushcfunction(L, ::process_profile);
      lua_setglobal(L, "process_profile");

      lua_pushcfunction(L, ::profile_pulley_translate);
      lua_setglobal(L, "profile_pulley_translate");

      lua_pushcfunction(L, ::process_identity);
      lua_setglobal(L, "process_identity");

      lua_pushcfunction(L, ::encoder_distance_estimation);
      lua_setglobal(L, "encoder_distance_estimation");

      lua_pushcfunction(L, ::sleep_ms);
      lua_setglobal(L, "sleep_ms");

      lua_pushcfunction(L, ::get_serial);
      lua_setglobal(L, "get_serial");

      lua_pushcfunction(L, time_str);
      lua_setglobal(L, "timeToString");

      lua_pushcfunction(L, get_now);
      lua_setglobal(L, "getNow");

      lua_pushcfunction(L, profile_decimation);
      lua_setglobal(L, "profile_decimation");

      lua_pushcfunction(L, ::alignProfile);
      lua_setglobal(L, "alignProfile");
      
      lua_pushcfunction(L, ::partitionProfile);
      lua_setglobal(L, "partitionProfile");
      
      lua_pushcfunction(L, ::prsToScan);
      lua_setglobal(L, "prsToScan");

      lua_pushcfunction(L, ::profilePartionToScan);
      lua_setglobal(L, "profilePartionToScan");

      lua_pushcfunction(L, ::extractPartition);
      lua_setglobal(L, "extractPartition");

      lua_pushcfunction(L, ::filterMsgs);
      lua_setglobal(L, "filterMsgs");

      lua_pushcfunction(L, ::fileCSV);
      lua_setglobal(L, "fileCSV");

      lua_pushcfunction(L, ::teeMsg);
      lua_setglobal(L, "teeMsg");

      lua_pushcfunction(L, ::voidMsg);
      lua_setglobal(L, "voidMsg");

      lua_pushcfunction(L, ::cadsLog);
      lua_setglobal(L, "cadsLog");

      return UL;
    }

  }

  std::tuple<Lua, bool> run_lua_code(std::string lua_code)
  {

    auto L = Lua{luaL_newstate(), lua_close};
    luaL_openlibs(L.get());

    L = lua::init(std::move(L), constants_device.Serial, lua_code);
    auto lua_status = luaL_dostring(L.get(), lua_code.c_str());

    if (lua_status != LUA_OK)
    {
      spdlog::get("cads")->error("{{ func = {},  msg = 'luaL_dostring: {}' }}", __func__, lua_tostring(L.get(), -1));
      return {std::move(L), true};
    }

    return {std::move(L), false};
  }

  std::tuple<Lua, bool> run_lua_config(std::string f)
  {
    namespace fs = std::filesystem;

    fs::path luafile;
    if (luascript_name)
    {
      luafile = luascript_name.value();
    }
    else
    {
      luafile = f;
      luafile.replace_extension("lua");
    }

    return run_lua_code(slurpfile(luafile.string()));
  }

  void push_externalmsg(lua_State *L, moodycamel::BlockingConcurrentQueue<std::tuple<std::string, std::string, std::string>> *queue)
  {
    lua_pushlightuserdata(L, queue);
    lua_pushcclosure(L, ::send_external_msg, 1);
  }
}