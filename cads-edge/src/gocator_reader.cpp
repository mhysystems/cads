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

  void GocatorReader::Start()
  {
    auto status = GoSystem_Start(m_system);

    if (kIsError(status))
    {
      throw runtime_error{"GoSensor_Start: "s + to_string(status)};
    }
    else
    {
      spdlog::get("cads")->info("GoSensor Starting");
    }
  }

  void GocatorReader::Stop()
  {
    auto status = GoSensor_Stop(m_sensor);
    m_gocatorFifo.enqueue({msgid::finished, 0});

    if (kIsError(status))
    {
      spdlog::get("cads")->error("GoSensor_Stop(m_sensor) -> {}", status);
    }
    else
    {
      spdlog::get("cads")->info("GoSensor Stopped");
    }
  }

  void GocatorReader::Log()
  {

    kObj(GoSensor, m_sensor);
    kAlloc tempAlloc = kObject_Alloc(m_sensor);
    kByte *fileData = kNULL;
    kSize fileSize = 0;

    GoSensor_IsReadable(m_sensor);

    GoControl_ReadFile(obj->control, GO_SENSOR_LIVE_LOG_NAME, &fileData, &fileSize, tempAlloc);

    // Uncomment to save transform to file (useful for debugging transform problems)
    kFile_Save("GoSensor.log", fileData, fileSize);

    kAlloc_Free(tempAlloc, fileData);
  }

  GocatorReader::GocatorReader(Io &gocatorFifo) : GocatorReaderBase(gocatorFifo)
  {
    m_assembly = CreateGoSdk();
    m_system = CreateGoSystem();
    auto sensor_count = GoSystem_SensorCount(m_system);

    if (sensor_count < 1)
    {
      throw runtime_error{"GoSystem_SensorCount: No sensor found"};
    }
    else
    {
      spdlog::get("cads")->info("Number of Camera's found: {}", sensor_count);
    }

    m_sensor = GoSystem_SensorAt(m_system, 0);

    auto status = GoSensor_Connect(m_sensor);

    if (kIsError(status))
    {
      throw runtime_error{"GoSensor_Connect: "s + to_string(status)};
    }

    if (kIsError(status = GoSystem_Stop(m_system)))
    {
      throw runtime_error{"GoSensor_Stop: "s + to_string(status)};
    }

    if (kIsError(status = GoSystem_EnableData(m_system, kTRUE)))
    {
      throw runtime_error{"GoSensor_EnableData: "s + to_string(status)};
    }

    auto setup = GoSensor_Setup(m_sensor);

    if (kNULL == setup)
    {
      throw runtime_error{"GoSensor_Setup: Invalid setup handle"};
    }

    if (kIsError(status = GoSetup_SetTriggerSource(setup, GO_TRIGGER_TIME)))
    {
      throw runtime_error{"GoSetup_SetTriggerSource: "s + to_string(status)};
    }

    if (kIsError(status = GoSetup_SetFrameRate(setup, constants_gocator.Fps)))
    {
      throw runtime_error{"GoSetup_SetFrameRate: "s + to_string(status)};
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

    if (kIsError(status = GoSystem_StartAlignment(m_system)))
    {
      throw runtime_error{"GoSetup_SetAlignmentStationaryTarget: "s + to_string(status)};
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
          if (GoAlignMsg_Status(message) == kOK)
          {
            spdlog::get("cads")->debug("{{func = {}, msg = {}}}", __func__, "Gocator is aligned");
          }
          else
          {
            spdlog::get("cads")->debug("{{func = {}, msg = {}}}", __func__, "Gocator is NOT aligned");
          }
        }
      }
      GoDestroy(dataset);
    }
    else
    {
      throw runtime_error{"GoSystem_StartAlignment: "s + to_string(status)};
    }

    if (kIsError(status = GoSystem_SetDataHandler(m_system, OnData, this)))
    {
      throw runtime_error{"GoSensor_SetDataHandler: "s + to_string(status)};
    }
  }

  kStatus kCall GocatorReader::OnData(kPointer context, GoSensor sensor, GoDataSet dataset)
  {
    auto me = static_cast<GocatorReader *>(context);

    if (!me->terminate)
    {
      me->OnData(sensor, dataset);
    }
    else
    {
      me->Stop();
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

  GocatorReader::~GocatorReader()
  {
    Stop();
    kStatus status = kOK;

    status = GoSensor_EnableData(m_sensor, kFALSE);
    if (kIsError(status))
    {
      spdlog::get("cads")->error("GoSensor_EnableData:{}", status);
    }
    status = GoSensor_Disconnect(m_sensor);
    if (kIsError(status))
    {
      spdlog::get("cads")->error("GoSensor_Disconnect:{}", status);
    }

    status = GoDestroy(m_system);
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

  kStatus GocatorReader::OnData([[maybe_unused]] GoSensor sensor, GoDataSet dataset)
  {
    double frame = 0.0;
    double y = 0.0;
    k16s *profile = 0;
    kSize profileWidth = 0;
    double xResolution = 0.0;
    double zResolution = 0.0;
    double xOffset = 0.0;
    double zOffset = 0.0;
    double yResolution = global_conveyor_parameters.TypicalSpeed / constants_gocator.Fps;

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

    if (true /*Todo move out of this function*/)
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
