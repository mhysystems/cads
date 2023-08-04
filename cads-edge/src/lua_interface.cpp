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
#include <filters.h>

namespace
{
  
  int send_external_msg(lua_State *L) {   
    auto p = (moodycamel::BlockingConcurrentQueue<std::tuple<std::string, std::string, std::string>> *)lua_topointer(L, lua_upvalueindex(1));
    std::string sub(lua_tostring(L,1));
    std::string cat(lua_tostring(L,2));
    std::string msg(lua_tostring(L,3));
    p->enqueue({sub,cat,msg});
    return 0;
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
      auto num_opt = tonumber(L,-1);
      lua_pop(L, 1);
      
      if (!num_opt)
      {
        spdlog::get("cads")->error("{{ func = {},  msg = 'Index {} not a number' }}", __func__,i);
        return std::nullopt;
      }else {
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
      auto vec = tonumbervector(L,-1);
      lua_pop(L, 1);
      
      if (!vec)
      {
        spdlog::get("cads")->error("{{ func = {},  msg = 'Index {} not an array' }}", __func__,i);
        return std::nullopt;
      }else {
        lua_array.push_back(*vec);
      }
    }

    return lua_array;
  }

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

std::optional<cads::Conveyor> toconveyor(lua_State *L, int index)
  {

    const std::string obj_name = "conveyor";

    if (!lua_istable(L, index))
    {
      spdlog::get("cads")->error("{{ func = {},  msg = '{} needs to be a table' }}", __func__,obj_name);
      return std::nullopt;
    }

    if (lua_getfield(L, index, "Id") == LUA_TNIL)
    {
      spdlog::get("cads")->error("{{ func = {},  msg = '{} requires {}' }}", __func__,obj_name,"Id");
      return std::nullopt;
    }

    auto id_opt = tointeger(L, -1);
    lua_pop(L, 1);

    if (!id_opt)
    {
      spdlog::get("cads")->error("{{ func = {},  msg = 'Id not a number' }}", __func__);
      return std::nullopt;
    }

    if (lua_getfield(L, index, "Org") == LUA_TNIL)
    {
      spdlog::get("cads")->error("{{ func = {},  msg = '{} requires {}' }}", __func__,obj_name,"Org");
      return std::nullopt;
    }

    auto org_opt = tostring(L, -1);
    lua_pop(L, 1);

    if (!org_opt)
    {
      spdlog::get("cads")->error("{{ func = {},  msg = 'Org not a string' }}", __func__);
      return std::nullopt;
    }

    if (lua_getfield(L, index, "Site") == LUA_TNIL)
    {
      spdlog::get("cads")->error("{{ func = {},  msg = '{} requires {}' }}", __func__,obj_name,"Site");
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
      spdlog::get("cads")->error("{{ func = {},  msg = '{} requires {}' }}", __func__,obj_name,"Name");
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
      spdlog::get("cads")->error("{{ func = {},  msg = '{} requires {}' }}", __func__,obj_name,"Timezone");
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
      spdlog::get("cads")->error("{{ func = {},  msg = '{} requires {}' }}", __func__,obj_name,"PulleyCircumference");
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
      spdlog::get("cads")->error("{{ func = {},  msg = '{} requires {}' }}", __func__,obj_name,"TypicalSpeed");
      return std::nullopt;
    }

    auto TypicalSpeed_opt = tonumber(L, -1);
    lua_pop(L, 1);

    if (!TypicalSpeed_opt)
    {
      spdlog::get("cads")->error("{{ func = {},  msg = 'TypicalSpeed not a number' }}", __func__);
      return std::nullopt;
    }


    if (lua_getfield(L, index, "Belt") == LUA_TNIL)
    {
      spdlog::get("cads")->error("{{ func = {},  msg = '{} requires {}' }}", __func__,obj_name,"Belt");
      return std::nullopt;
    }

    auto Belt_opt = tointeger(L, -1);
    lua_pop(L, 1);

    if (!Belt_opt)
    {
      spdlog::get("cads")->error("{{ func = {},  msg = 'Belt not a integer' }}", __func__);
      return std::nullopt;
    }

    if (lua_getfield(L, index, "Length") == LUA_TNIL)
    {
      spdlog::get("cads")->error("{{ func = {},  msg = '{} requires {}' }}", __func__,obj_name,"Length");
      return std::nullopt;
    }

    auto Length_opt = tonumber(L, -1);
    lua_pop(L, 1);

    if (!Length_opt)
    {
      spdlog::get("cads")->error("{{ func = {},  msg = 'Length not a number' }}", __func__);
      return std::nullopt;
    }

    if (lua_getfield(L, index, "WidthN") == LUA_TNIL)
    {
      spdlog::get("cads")->error("{{ func = {},  msg = '{} requires {}' }}", __func__,obj_name,"WidthN");
      return std::nullopt;
    }

    auto WidthN_opt = tointeger(L, -1);
    lua_pop(L, 1);

    if (!WidthN_opt)
    {
      spdlog::get("cads")->error("{{ func = {},  msg = 'WidthN not a number' }}", __func__);
      return std::nullopt;
    }

    return  cads::Conveyor{*id_opt,*org_opt,*site_opt,*name_opt,*Timezone_opt,*PulleyCircumference_opt,*TypicalSpeed_opt,*Belt_opt,*Length_opt,*WidthN_opt};
  }
  
  std::optional<cads::Dbscan> todbscan(lua_State *L, int index)
  {
    const std::string obj_name = "dbscan";

    if (!lua_istable(L, index))
    {
      spdlog::get("cads")->error("{{ func = {},  msg = '{} needs to be a table' }}", __func__,obj_name);
      return std::nullopt;
    }

    if (lua_getfield(L, index, "InClusterRadius") == LUA_TNIL)
    {
      spdlog::get("cads")->error("{{ func = {},  msg = '{} requires {}' }}", __func__,obj_name,"InClusterRadius");
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
      spdlog::get("cads")->error("{{ func = {},  msg = '{} requires {}' }}", __func__,obj_name,"MinPoints");
      return std::nullopt;
    }

    auto MinPoints_opt = tointeger(L, -1);
    lua_pop(L, 1);

    if (!MinPoints_opt)
    {
      spdlog::get("cads")->error("{{ func = {},  msg = 'MinPoints not a number' }}", __func__);
      return std::nullopt;
    }

    return cads::Dbscan{*InClusterRadius_opt,*MinPoints_opt};
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
      spdlog::get("cads")->error("{{ func = {},  msg = '{} requires {}' }}", __func__,obj_name,"Skip");
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
      spdlog::get("cads")->error("{{ func = {},  msg = '{} requires {}' }}", __func__,obj_name,"Delay");
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
      spdlog::get("cads")->error("{{ func = {},  msg = '{} requires {}' }}", __func__,obj_name,"Sos");
      return std::nullopt;
    }

    auto sos_opt = to2Dnumbervector(L, -1);
    lua_pop(L, 1);

    if (!sos_opt)
    {
      spdlog::get("cads")->error("{{ func = {},  msg = 'Sos not a 2D array of numbers' }}", __func__);
      return std::nullopt;
    }

    if((*sos_opt).size() != 10) {
      spdlog::get("cads")->error("{{ func = {},  msg = 'Sos array requires 10 sub arrays' }}", __func__);
      return std::nullopt;
    }

    for(auto e : *sos_opt) {
      if(e.size() != 6) {
        spdlog::get("cads")->error("{{ func = {},  msg = 'Sos sub array requires 6 numbers' }}", __func__);
        return std::nullopt;       
      }
    }

    return cads::IIRFilterConfig{*skip_opt,*delay_opt,*sos_opt};
  }

  std::optional<cads::RevolutionSensorConfig> torevolutionsensor(lua_State *L, int index)
  {
    const std::string obj_name = "revolutionsensor";

    if (!lua_istable(L, index))
    {
      spdlog::get("cads")->error("{{ func = {},  msg = '{} needs to be a table' }}", __func__,obj_name);
      return std::nullopt;
    }

    if (lua_getfield(L, index, "Source") == LUA_TNIL)
    {
      spdlog::get("cads")->error("{{ func = {},  msg = '{} requires {}' }}", __func__,obj_name,"Source");
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
      spdlog::get("cads")->error("{{ func = {},  msg = '{} requires {}' }}", __func__,obj_name,"TriggerDistance");
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
      spdlog::get("cads")->error("{{ func = {},  msg = '{} requires {}' }}", __func__,obj_name,"Bias");
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
      spdlog::get("cads")->error("{{ func = {},  msg = '{} requires {}' }}", __func__,obj_name,"Threshold");
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
      spdlog::get("cads")->error("{{ func = {},  msg = '{} requires {}' }}", __func__,obj_name,"Bidirectional");
      return std::nullopt;
    }

    bool bidirectional = lua_toboolean(L, -1);
    lua_pop(L, 1);

    if (lua_getfield(L, index, "Skip") == LUA_TNIL)
    {
      spdlog::get("cads")->error("{{ func = {},  msg = '{} requires {}' }}", __func__,obj_name,"Skip");
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
    if(*source_opt == "raw") {
      source = cads::RevolutionSensorConfig::Source::height_raw;
    }else if(*source_opt  == "length") {
      source = cads::RevolutionSensorConfig::Source::length;
    }else {
      source = cads::RevolutionSensorConfig::Source::height_filtered;  
    }

    return cads::RevolutionSensorConfig{source,*trigger_dis_opt,*bias_opt,*theshold_opt,bidirectional,*skip_opt};
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

    if (lua_getfield(L, index, "PulleySamplesExtend") == LUA_TNIL)
    {
      spdlog::get("cads")->error("{{ func = {},  msg = 'profile config requires {}' }}", __func__, "PulleySamplesExtend");
      return std::nullopt;
    }

    auto pulley_sample_extend_opt = tointeger(L, -1);
    lua_pop(L, 1);

    if (!pulley_sample_extend_opt)
    {
      spdlog::get("cads")->error("{{ func = {},  msg = 'PulleySamplesExtend not a integer' }}", __func__);
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


    if (lua_getfield(L, index, "Conveyor") == LUA_TNIL)
    {
      spdlog::get("cads")->error("{{ func = {},  msg = 'profile config requires {}' }}", __func__, "Conveyor");
      return std::nullopt;
    }

    auto conveyor_opt = toconveyor(L, -1);
    lua_pop(L, 1);

    if (!conveyor_opt)
    {
      spdlog::get("cads")->error("{{ func = {},  msg = 'Conveyor not a Conveyor' }}", __func__);
      return std::nullopt;
    }

    if (lua_getfield(L, index, "Dbscan") == LUA_TNIL)
    {
      spdlog::get("cads")->error("{{ func = {},  msg = 'profile config requires {}' }}", __func__, "Dbscan");
      return std::nullopt;
    }

    auto Dbscan_opt = todbscan(L, -1);
    lua_pop(L, 1);

    if (!Dbscan_opt)
    {
      spdlog::get("cads")->error("{{ func = {},  msg = 'Dbscan not a Dbscan' }}", __func__);
      return std::nullopt;
    }

    return cads::ProfileConfig{*width_opt,*nanpercentage_opt,*clipheight_opt,*iirfilter_opt,*pulley_sample_extend_opt,*revolution_sensor_opt,*conveyor_opt,*Dbscan_opt};
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

    auto range_opt = topair(L, -1);
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

    if (lua_getfield(L, index, "Delay") == LUA_TNIL)
    {
      spdlog::get("cads")->error("{{ func = {},  msg = 'sqlite gocator Config requires {}' }}", __func__, "Delay");
      return std::nullopt;
    }

    auto delay_opt = tonumber(L, -1);
    lua_pop(L, 1);

    if (!delay_opt)
    {
      spdlog::get("cads")->error("{{ func = {},  msg = 'delay not a number' }}", __func__);
      return std::nullopt;
    }

    
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

    return cads::SqliteGocatorConfig{*range_opt, *fps_opt, forever, *delay_opt, *source_opt, *typical_speed_opt};
  }

  std::optional<cads::GocatorConfig> togocatorconfig(lua_State *L, int index)
  {
    const std::string obj_name = "gocatorconfig";

    if (!lua_istable(L, index))
    {
      spdlog::get("cads")->error("{{ func = {},  msg = '{} needs to be a lua table' }}", __func__,obj_name);
      return std::nullopt;
    }

    if (lua_getfield(L, index, "Trim") == LUA_TNIL)
    {
      spdlog::get("cads")->error("{{ func = {},  msg = '{} requires {}' }}", __func__, obj_name,"Trim");
      return std::nullopt;
    }

    lua_getfield(L, index, "Trim");
    bool trim = lua_toboolean(L, -1);
    lua_pop(L, 1);

    if (lua_getfield(L, index, "TypicalResolution") == LUA_TNIL)
    {
      spdlog::get("cads")->error("{{ func = {},  msg = '{} requires {}' }}", __func__, obj_name,"TypicalResolution");
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
      spdlog::get("cads")->error("{{ func = {},  msg = '{} needs to be a lua table' }}", __func__,obj_name);
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

    return cads::DynamicProcessingConfig{*WidthN_opt};
  }

  void pushmetric(lua_State *L, cads::Measure::MeasureMsg m) 
  {
      auto [sub,quality,time,value] = m;

      lua_newtable(L);
      lua_pushnumber(L, 1); 
      lua_pushstring(L,sub.c_str());
      lua_settable(L,-2);

      lua_pushnumber(L, 2); 
      lua_pushnumber(L,quality);
      lua_settable(L,-2);
      
      lua_pushnumber(L, 3); 
      auto tp = std::chrono::duration<double>(get<2>(m).time_since_epoch());
      lua_pushnumber(L, tp.count());
      lua_settable(L,-2);
            
      switch (value.index()) {
        case 0:
          lua_pushnumber(L, 4); 
          lua_pushnumber(L, get<double>(value)); 
          lua_settable(L,-2);     
          break;
        case 1:
          lua_pushnumber(L, 4);
          lua_pushstring(L, get<std::string>(value).c_str());
          lua_settable(L,-2);
          break;
        case 2: 
          lua_pushnumber(L, 4);
          lua_pushlightuserdata(L, &value);
          lua_pushcclosure(L, execute_func2<double>, 1);
          lua_settable(L,-2);
          break;
        case 3: 
          lua_pushnumber(L, 4);
          lua_pushlightuserdata(L, &value);
          lua_pushcclosure(L, execute_func2<std::string>, 1);
          lua_settable(L,-2);
          break;
        case 4: {
          auto [v,location] = get<std::tuple<double,double>>(value);
          lua_pushnumber(L, 4);
          lua_pushnumber(L, v);
          lua_settable(L,-2);
          lua_pushnumber(L, 5); 
          lua_pushnumber(L, location); 
          lua_settable(L,-2);
          break;
          
        }
        default: break;
      }

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
    auto q = static_cast<cads::Io *>(lua_touserdata(L, 1));
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
    return 0;
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

  int BlockingReaderWriterQueue(lua_State *L)
  {
    new (lua_newuserdata(L, sizeof(cads::Adapt<moodycamel::BlockingReaderWriterQueue<cads::msg>>))) cads::Adapt<moodycamel::BlockingReaderWriterQueue<cads::msg>>(moodycamel::BlockingReaderWriterQueue<cads::msg>());

    lua_createtable(L, 0, 1);
    lua_pushcfunction(L, Io_gc);
    lua_setfield(L, -2, "__gc");
    lua_setmetatable(L, -2);

    return 1;
  }

  int wait_for(lua_State *L)
  {
    auto io = static_cast<cads::Io *>(lua_touserdata(L, 1));
    auto s = lua_tointeger(L, 2);

    cads::msg m;
    auto have_value = io->wait_dequeue_timed(m, std::chrono::seconds(s));
    lua_pushboolean(L, have_value);
    auto mid = std::get<0>(m);
    lua_pushinteger(L, mid);

    if(mid == cads::msgid::realtime_metric) {
      pushmetric(L,std::get<cads::Measure::MeasureMsg>(std::get<1>(m)));
      return 3;
    }

    return 2;
  }

  int mk_thread(lua_State *L, std::function<void(cads::Io &)> fn)
  {
    auto q = static_cast<cads::Io *>(lua_touserdata(L, -1));
    new (lua_newuserdata(L, sizeof(std::thread))) std::thread(fn, std::ref(*q));
    lua_createtable(L, 0, 1);
    lua_pushcfunction(L, thread_gc);
    lua_setfield(L, -2, "__gc");
    lua_setmetatable(L, -2);
    return 1;
  }

  int mk_thread2(lua_State *L, std::function<void(cads::Io &, cads::Io &)> fn)
  {
    auto in = static_cast<cads::Io *>(lua_touserdata(L, -2));
    auto out = static_cast<cads::Io *>(lua_touserdata(L, -1));

    new (lua_newuserdata(L, sizeof(std::thread))) std::thread(fn, std::ref(*in), std::ref(*out));
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
    (*gocator)->Start(fps);

    return 0;
  }

  int gocator_stop(lua_State *L)
  {
    auto gocator = static_cast<std::unique_ptr<cads::GocatorReaderBase> *>(lua_touserdata(L, -1));
    (*gocator)->Stop();

    return 0;
  }

  int gocator_gc(lua_State *L)
  {
    auto gocator = static_cast<std::unique_ptr<cads::GocatorReaderBase> *>(lua_touserdata(L, -1));
    gocator->~unique_ptr<cads::GocatorReaderBase>();

    return 0;
  }

  int sqlitegocator(lua_State *L)
  {
    auto sqlite_gocator_config_opt = tosqlitegocatorconfig(L, 1);

    if (!sqlite_gocator_config_opt)
    {
      lua_pushnil(L);
      return 1;
    }

    auto q = static_cast<cads::Io *>(lua_touserdata(L, 2));
    auto p = new (lua_newuserdata(L, sizeof(std::unique_ptr<cads::GocatorReaderBase>))) std::unique_ptr<cads::SqliteGocatorReader>;
    *p = std::make_unique<cads::SqliteGocatorReader>(*sqlite_gocator_config_opt, *q);

    lua_createtable(L, 0, 1);
    lua_pushcfunction(L, gocator_gc);
    lua_setfield(L, -2, "__gc");
    lua_createtable(L, 0, 2);
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
    auto gocator_config_opt = togocatorconfig(L, 1);

    if (!gocator_config_opt)
    {
      lua_pushnil(L);
      return 1;
    }

    auto q = static_cast<cads::Io *>(lua_touserdata(L, 2));
    auto p = new (lua_newuserdata(L, sizeof(std::unique_ptr<cads::GocatorReaderBase>))) std::unique_ptr<cads::GocatorReader>;
    *p = std::make_unique<cads::GocatorReader>(*gocator_config_opt, *q);

    lua_createtable(L, 0, 1);
    lua_pushcfunction(L, gocator_gc);
    lua_setfield(L, -2, "__gc");
    lua_createtable(L, 0, 2);
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

  int window_processing_thread(lua_State *L)
  {
    using namespace std::placeholders;

    auto conveyor = toconveyor(L, 1);
    auto bound = std::bind(cads::window_processing_thread, *conveyor, _1, _2);
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

    auto conveyor = toconveyor(L, 1);
    auto bound = std::bind(cads::save_send_thread, *conveyor, _1, _2);
    return mk_thread2(L, bound);
  }

  int loop_beltlength_thread(lua_State *L)
  {
    using namespace std::placeholders;

    auto conveyor = toconveyor(L, 1);
    auto bound = std::bind(cads::loop_beltlength_thread, *conveyor, _1, _2);
    return mk_thread2(L, bound);
  }

  int process_profile(lua_State *L)
  {
    using namespace std::placeholders;
    auto profile_config = toprofileconfig(L,1);
    auto bound = std::bind(cads::process_profile, *profile_config, _1, _2);
    return mk_thread2(L, bound);
  }

  int process_identity(lua_State *L)
  {
    return mk_thread2(L, cads::process_identity);
  }

  int encoder_distance_estimation(lua_State *L)
  {

    auto next = static_cast<cads::Io *>(lua_touserdata(L, 1));
    double stride = lua_tonumber(L, 2);
    new (lua_newuserdata(L, sizeof(cads::Adapt<decltype(cads::encoder_distance_estimation(std::ref(*next), stride))>))) cads::Adapt<decltype(cads::encoder_distance_estimation(std::ref(*next), stride))>(cads::encoder_distance_estimation(std::ref(*next), stride));

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

}

namespace cads
{
  namespace lua
  {

    Lua init(Lua UL)
    {
      auto L = UL.get();

      lua_pushcfunction(L, ::BlockingReaderWriterQueue);
      lua_setglobal(L, "BlockingReaderWriterQueue");

      lua_pushcfunction(L, ::anomaly_detection_thread);
      lua_setglobal(L, "anomaly_detection_thread");

      lua_pushcfunction(L, ::save_send_thread);
      lua_setglobal(L, "save_send_thread");

      lua_pushcfunction(L, ::window_processing_thread);
      lua_setglobal(L, "window_processing_thread");

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

      lua_pushcfunction(L, ::process_identity);
      lua_setglobal(L, "process_identity");

      lua_pushcfunction(L, ::encoder_distance_estimation);
      lua_setglobal(L, "encoder_distance_estimation");

      lua_pushcfunction(L, ::sleep_ms);
      lua_setglobal(L, "sleep_ms");

      lua_pushcfunction(L, ::get_serial);
      lua_setglobal(L, "get_serial");

      return UL;
    }

  }

  std::tuple<Lua, bool> run_lua_code(std::string lua_code)
  {

    auto L = Lua{luaL_newstate(), lua_close};
    luaL_openlibs(L.get());

    L = lua::init(std::move(L));
    auto lua_status = luaL_dostring(L.get(), lua_code.c_str());

    if (lua_status != LUA_OK)
    {
      spdlog::get("cads")->error("{{ func = {},  msg = 'luaL_dofile: {}' }}", __func__, lua_tostring(L.get(), -1));
      return {std::move(L), true};
    }

    return {std::move(L), false};
  }

  std::tuple<Lua, bool> run_lua_config(std::string f)
  {
    namespace fs = std::filesystem;

    fs::path luafile{f};
    luafile.replace_extension("lua");

    auto L = Lua{luaL_newstate(), lua_close};
    luaL_openlibs(L.get());

    L = lua::init(std::move(L));

    auto lua_status = luaL_dofile(L.get(), luafile.string().c_str());

    if (lua_status != LUA_OK)
    {
      spdlog::get("cads")->error("{}: luaL_dofile: {}", __func__, lua_tostring(L.get(), -1));
      return {std::move(L), true};
    }

    return {std::move(L), false};
  }

  void push_externalmsg(lua_State *L, moodycamel::BlockingConcurrentQueue<std::tuple<std::string, std::string, std::string>> *queue)
  {
    lua_pushlightuserdata(L, queue);
    lua_pushcclosure(L, ::send_external_msg, 1);
  }
}