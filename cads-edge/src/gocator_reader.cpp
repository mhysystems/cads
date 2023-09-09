#include "gocator_reader.h"

#include <GoSdk/GoSdk.h>
#include <kApi/kApiDef.h>

#include <algorithm>
#include <bits/stdint-uintn.h>
#include <ios>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <thread>
#include <vector>

#include <spdlog/spdlog.h>

using namespace std;
using namespace std::chrono;

namespace cads
{

  namespace
  {

    template <class T>
    decltype(auto) nmToMm(T in)
    {
      return in / 1e6;
    }

    template <class T>
    decltype(auto) umToMm(T in)
    {
      return in / 1e3;
    }

    auto CreateGoSdk()
    {
      kAssembly assembly = kNULL;
      auto status = GoSdk_Construct(&assembly);

      if (kIsError(status))
      {
        throw runtime_error{"GoSdk_Construct: "s + to_string(status)};
      }

      return assembly;
    }

    auto CreateGoSystem()
    {
      GoSystem system = kNULL;
      auto status = GoSystem_Construct(&system, kNULL);

      if (kIsError(status))
      {
        throw runtime_error{"GoSystem_Construct: "s + to_string(status)};
      }

      return system;
    }

  }

  bool GocatorReader::Start_impl()
  {
    if(!m_stopped) return false;
    
    auto status = GoSystem_SetDataHandler(m_system, OnData, this);
    
    if (kIsError(status))
    {
      return true;
    }
  
    status = GoSystem_Start(m_system);

    if (kIsError(status))
    {
      return true;
    }
    else
    {
      m_stopped = false;
      spdlog::get("cads")->info("GoSensor Starting");
    }

    return false;
  }

  void GocatorReader::Stop_impl(bool signal_finished)
  {
    if(m_stopped) return;
    
    auto status = GoSystem_Stop(m_system);
    
    if (kIsError(status))
    {
      spdlog::get("cads")->error(R"({{where = '{}', id = '{}', value = '{}', msg = '{}'}})", __func__,"GoSystem_Stop"s,status,"Gocator error");
    }
    else
    {
      if(signal_finished) {
        m_gocatorFifo.enqueue({msgid::finished, 0});
      }

      m_stopped = true;
      spdlog::get("cads")->info(R"({{where = '{}', id = '{}', value = '{}', msg = '{}'}})", __func__,"GoSensor Stopped"s,"empty"s,"GoSensor Stopped"s);
    }
  }

  void GocatorReader::Log()
  {

    auto sensor = GoSystem_SensorAt(m_system, 0);
    kObj(GoSensor, sensor);
    kAlloc tempAlloc = kObject_Alloc(sensor);
    kByte *fileData = kNULL;
    kSize fileSize = 0;

    GoSensor_IsReadable(sensor);

    GoControl_ReadFile(obj->control, GO_SENSOR_LIVE_LOG_NAME, &fileData, &fileSize, tempAlloc);

    // Uncomment to save transform to file (useful for debugging transform problems)
    kFile_Save("GoSensor.log", fileData, fileSize);

    kAlloc_Free(tempAlloc, fileData);
  }

  void GocatorReader::LaserOff()
  {
    auto assembly = CreateGoSdk();
    auto system = CreateGoSystem();
    GoSystem_Stop(system);
    GoDestroy(system);
    GoDestroy(assembly);
  }

  bool GocatorReader::SetFrameRate(double fps) {
    
    auto sensor = GoSystem_SensorAt(m_system, 0);
    auto setup = GoSensor_Setup(sensor);
    auto status = GoSetup_SetFrameRate(setup, fps);
    return kIsError(status);
  }

  bool GocatorReader::SetFoV_impl(double len)
  {
    auto sensor = GoSystem_SensorAt(m_system, 0);
    auto setup = GoSensor_Setup(sensor);
    auto role = GoSensor_Role(sensor);

    auto max = GoSetup_ActiveAreaHeightLimitMax(setup,role);
    auto min = GoSetup_ActiveAreaHeightLimitMin(setup,role);

    if(len < min) len = min;
    if(len > max) len = max;

    spdlog::get("cads")->debug(R"({{where = '{}', id = '{}', value = '{}', msg = '{}'}})", __func__,"FoV length"s,len,"Set gocators field of view"s);
    
    auto status = GoSetup_SetActiveAreaHeight(setup, role, len);
    if(kIsError(status))
    {
      spdlog::get("cads")->error(R"({{where = '{}', id = '{}', value = '{}', msg = '{}'}})", __func__,"GoSetup_SetActiveAreaHeight"s,status,"Gocator error");
    }

    return kIsError(status);
  }

  bool GocatorReader::Align_impl() {

    auto status = GoSystem_SetDataHandler(m_system, kNULL, kNULL);
    
    if (kIsError(status))
    {
      spdlog::get("cads")->error(R"({{where = '{}', id = '{}', value = '{}', msg = '{}'}})", __func__,"GoSystem_SetDataHandler"s,status,"Gocator error");
      return true;
    }
    
    status = GoSystem_StartAlignment(m_system);

    if (kIsError(status))
    {
      spdlog::get("cads")->error(R"({{where = '{}', id = '{}', value = '{}', msg = '{}'}})", __func__,"GoSystem_StartAlignment"s,status,"Gocator error");
      return true;
    }

    const auto timeout_us = 30000000;
    GoDataSet dataset = nullptr;
    if ((status = GoSystem_ReceiveData(m_system, &dataset, timeout_us)) == kOK)
    {
      for (kSize i = 0; i < GoDataSet_Count(dataset); ++i)
      {
        GoDataMsg message = GoDataSet_At(dataset, i);
        if (GoDataMsg_Type(message) == GO_DATA_MESSAGE_TYPE_ALIGNMENT)
        {
          if ((status = GoAlignMsg_Status(message)) == kOK)
          {
            spdlog::get("cads")->debug(R"({{func = '{}', msg = '{}'}})", __func__, "Gocator is aligned");
          }
          else
          {
            spdlog::get("cads")->debug(R"({{func = '{}', msg = '{}'}})", __func__, "Gocator is NOT aligned");
            return true;
          }
        }
      }
      GoDestroy(dataset);
    }
    else
    {
      throw runtime_error{"GoSystem_StartAlignment: "s + to_string(status)};
    }

    return kIsError(status);
  }

  GocatorReader::GocatorReader(GocatorConfig cnf, Io &gocatorFifo) : GocatorReaderBase(gocatorFifo), config(cnf)
  {
    
    static_assert(std::is_same_v<double, k64f> == true);

    m_assembly = CreateGoSdk();
    m_system = CreateGoSystem();

    auto status = GoSystem_Connect(m_system);

    if (kIsError(status))
    {
      throw runtime_error{"GoSystem_Connect: "s + to_string(status)};
    }

    if (kIsError(status = GoSystem_Stop(m_system)))
    {
      throw runtime_error{"GoSystem_Stop: "s + to_string(status)};
    }

    if (kIsError(status = GoSystem_EnableData(m_system, kTRUE)))
    {
      throw runtime_error{"GoSystem_EnableData: "s + to_string(status)};
    }

    auto sensor_count = GoSystem_SensorCount(m_system);

    if (sensor_count < 1)
    {
      throw runtime_error{"GoSystem_SensorCount: No sensor found"};
    }
    else
    {
      spdlog::get("cads")->info("Number of Camera's found: {}", sensor_count);
    }

    auto sensor = GoSystem_SensorAt(m_system, 0);
    auto setup = GoSensor_Setup(sensor);

    if (kNULL == setup)
    {
      throw runtime_error{"GoSensor_Setup: Invalid setup handle"};
    }

    if (kIsError(status = GoSetup_SetTriggerSource(setup, GO_TRIGGER_TIME)))
    {
      throw runtime_error{"GoSetup_SetTriggerSource: "s + to_string(status)};
    }

    if (kIsError(status = GoSetup_EnableUniformSpacing(setup, kTRUE)))
    {
      throw runtime_error{"GoSetup_EnableUniformSpacing: "s + to_string(status)};
    }

    if (kIsError(status = GoSetup_SetAlignmentType(setup, GO_ALIGNMENT_TYPE_STATIONARY)))
    {
      throw runtime_error{"GoSetup_SetAlignmentType: "s + to_string(status)};
    }

    if (kIsError(status = GoSetup_SetAlignmentStationaryTarget(setup, GO_ALIGNMENT_TARGET_NONE)))
    {
      throw runtime_error{"GoSetup_SetAlignmentStationaryTarget: "s + to_string(status)};
    }

  }

  GocatorReader::~GocatorReader()
  {
    Stop_impl(true);

    auto status = GoDestroy(m_system);
    if (kIsError(status))
    {
      spdlog::get("cads")->error("GoDestroy system:{}", status);
    }

    status = GoDestroy(m_assembly);
    if (kIsError(status))
    {
      spdlog::get("cads")->error("GoDestroy assembly:{}", status);
    }
  }

  kStatus kCall GocatorReader::OnData(kPointer context, [[maybe_unused]] GoSystem system, GoDataSet dataset)
  {
    auto me = static_cast<GocatorReader *>(context);

    if (!me->m_stopped)
    {
      me->OnData(dataset);
    }

    GoDestroy(dataset);
    if (me->m_gocatorFifo.size_approx() > me->m_buffer_size_warning)
    {
      spdlog::get("cads")->error("Cads {}: Showing signs of not being able to keep up with data source. Size {}", __func__, me->m_buffer_size_warning);
      me->m_buffer_size_warning += 4096;
    }

    return kOK;
  }

  kStatus kCall GocatorReader::OnSystem(kPointer context, GoSystem system, GoDataSet data)
  {
    return static_cast<GocatorReader *>(context)->OnSystem(system, data);
  }

  kStatus GocatorReader::OnSystem([[maybe_unused]] GoSystem system, GoDataSet dataset)
  {
    for (kSize i = 0; i < GoDataSet_Count(dataset); ++i)
    {
      GoHealthMsg message = GoDataSet_At(dataset, i);
      for (kSize k = 0; k < GoHealthMsg_Count(message); k++)
      {
        auto healthIndicator = GoHealthMsg_At(message, k);
        if (healthIndicator->id == GO_HEALTH_ENCODER_VALUE)
          spdlog::get("cads")->info("Indicator[{}]: Id:{} Instance:{} Value:{}", k, healthIndicator->id, healthIndicator->instance, healthIndicator->value);
      }
    }

    GoDestroy(dataset);

    return kOK;
  }

  kStatus GocatorReader::OnData(GoDataSet dataset)
  {
    double frame = 0.0;
    double y = 0.0;
    k16s *profile = 0;
    kSize profileWidth = 0;
    double xResolution = 0.0;
    double zResolution = 0.0;
    double xOffset = 0.0;
    double zOffset = 0.0;
    double yResolution = config.TypicalResolution;

    for (kSize i = 0; i < GoDataSet_Count(dataset); ++i)
    {
      auto message = GoDataSet_At(dataset, i);

      switch (GoDataMsg_Type(message))
      {
      case GO_DATA_MESSAGE_TYPE_STAMP:
      {
        for (kSize j = 0; j < GoStampMsg_Count(message); ++j)
        {
          GoStamp *goStamp = GoStampMsg_At(message, j);
          frame = (double)goStamp->frameIndex;
        }
      }
      break;
      case GO_DATA_MESSAGE_TYPE_UNIFORM_PROFILE:
      {
        auto num = GoUniformProfileMsg_Count(message);
        for (kSize j = 0; j < num; ++j)
        {
          profile = GoUniformProfileMsg_At(message, j);
          profileWidth = GoUniformProfileMsg_Width(message);
          xResolution = nmToMm(GoUniformProfileMsg_XResolution(message));
          zResolution = nmToMm(GoUniformProfileMsg_ZResolution(message));
          xOffset = umToMm(GoUniformProfileMsg_XOffset(message));
          zOffset = umToMm(GoUniformProfileMsg_ZOffset(message));
        }
      }
      break;
      }
    }

    if (m_first_frame)
    {
      m_first_frame = false;
      m_gocatorFifo.enqueue({msgid::gocator_properties, GocatorProperties{xResolution, zResolution, zOffset}});
    }

    y = (frame - 1) * yResolution;

    // Trim invalid values
    auto profile_end = profile + profileWidth;
    auto profile_begin = profile;

    if (config.Trim)
    {
      for (; profile_end > profile && *(profile_end - 1) == k16S_NULL; --profile_end)
      {
      }

      for (; profile_begin < profile_end && *profile_begin == k16S_NULL; ++profile_begin)
      {
      }
    }

    auto samples_width = (double)distance(profile, profile_begin);
    auto z = GocatorReaderBase::k16sToFloat(profile_begin, profile_end, zResolution, zOffset);

    m_gocatorFifo.enqueue({msgid::scan, cads::profile{std::chrono::high_resolution_clock::now(), y, xOffset + samples_width * xResolution, std::move(z)}});


    return kOK;
  }

}
