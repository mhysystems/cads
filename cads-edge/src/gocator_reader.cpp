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
    auto status = GoSensor_Start(m_sensor);

    if (kIsError(status))
    {
      throw runtime_error{"GoSensor_Start: "s + to_string(status)};
    }else{
      spdlog::get("gocator")->info("GoSensor Starting");
    }
  }

  void GocatorReader::Stop()
  {
    
    auto status = GoSensor_Stop(m_sensor);

    if (kIsError(status))
    {
      spdlog::get("gocator")->error("GoSensor_Stop(m_sensor) -> {}", status);
    } else {
      spdlog::get("gocator")->info("GoSensor Stopped");
    }
  }

  void GocatorReader::Log() {
    
    kObj(GoSensor, m_sensor);
    kAlloc tempAlloc = kObject_Alloc(m_sensor);
    kByte* fileData = kNULL;
    kSize fileSize = 0;

    GoSensor_IsReadable(m_sensor);

    GoControl_ReadFile(obj->control, GO_SENSOR_LIVE_LOG_NAME, &fileData, &fileSize, tempAlloc);

        // Uncomment to save transform to file (useful for debugging transform problems)
    kFile_Save("GoSensor.log", fileData, fileSize);

    kAlloc_Free(tempAlloc, fileData);

  }

  GocatorReader::GocatorReader(moodycamel::BlockingReaderWriterQueue<msg> &gocatorFifo, std::string ip_add) : GocatorReaderBase(gocatorFifo)
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
      spdlog::get("gocator")->info("Number of Camera's found: {}", sensor_count);
    }

    if (!ip_add.empty())
    {
      kIpAddress gocator_ip;
      auto status = kIpAddress_Parse(&gocator_ip, ip_add.c_str());

      if (kIsError(status))
      {
        throw runtime_error{"Illformed Ip Address: "s + to_string(status)};
      }

      status = GoSystem_FindSensorByIpAddress(m_system, &gocator_ip, &m_sensor);

      if (kIsError(status))
      {
        throw runtime_error{"Cannot connect to gocator via IP: "s + to_string(status)};
      }
    }
    else
    {
      m_sensor = GoSystem_SensorAt(m_system, 0);
    }

    auto status = GoSensor_Connect(m_sensor);
    
    if (kIsError(status))
    {
      throw runtime_error{"GoSensor_Connect: "s + to_string(status)};
    }
    if (kIsError(status = GoSensor_Stop(m_sensor)))
    {
      throw runtime_error{"GoSensor_Stop: "s + to_string(status)};
    }

    if (kIsError(status = GoSensor_SetDataHandler(m_sensor, OnData, this)))
    {
      throw runtime_error{"GoSensor_SetDataHandler: "s + to_string(status)};
    }

    if (kIsError(status = GoSensor_EnableData(m_sensor, kTRUE)))
    {
      throw runtime_error{"GoSensor_EnableData: "s + to_string(status)};
    }

#if 0
                   
  if (kIsError(status = GoSystem_SetHealthHandler(m_system, OnSystem, this)))
	{
			throw runtime_error{"GoSensor_SetDataHandler: "s + to_string(status)};
	}

  if (kIsError(status = GoSensor_EnableHealth(m_sensor, kTRUE)))
	{
			throw runtime_error{"GoSensor_EnableHealth: "s + to_string(status)};
	}
#endif

    auto setup = GoSensor_Setup(m_sensor);

    if (kNULL == setup)
    {
      throw runtime_error{"GoSensor_Setup: Invalid setup handle"};
    }

    if (kIsError(status = GoSetup_EnableUniformSpacing(setup, kTRUE)))
    {
      throw runtime_error{"GoSetup_EnableUniformSpacing: "s + to_string(status)};
    }

    m_encoder_resolution = (double)GoTransform_EncoderResolution(GoSensor_Transform(m_sensor));
    m_yResolution = GoSetup_EncoderSpacing(setup);
    m_frame_rate = GoSetup_FrameRate(setup);
  }

  GocatorReader::GocatorReader(moodycamel::BlockingReaderWriterQueue<msg> &gocatorFifo, bool use_encoder, std::string ip_add) : GocatorReader(gocatorFifo, ip_add)
  {
    m_use_encoder = use_encoder;
  }

  kStatus kCall GocatorReader::OnData(kPointer context, GoSensor sensor, GoDataSet dataset)
  {
    auto me = static_cast<GocatorReader *>(context);
    
    if(!me->terminate) {
      return me->OnData(sensor, dataset);
    }else {
        me->m_gocatorFifo.enqueue({msgid::finished, 0});
    }
    
    GoDestroy(dataset);
    if (me->m_gocatorFifo.size_approx() > me->m_buffer_size_warning)
    {
      spdlog::get("gocator")->error("Cads Showing signs of not being able to keep up with data source. Size {}", me->m_buffer_size_warning);
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
    GoSensor_Disconnect(m_sensor);
    GoDestroy(m_system);
    GoDestroy(m_assembly);
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
          spdlog::get("gocator")->info("Indicator[{}]: Id:{} Instance:{} Value:{}", k, healthIndicator->id, healthIndicator->instance, healthIndicator->value);
      }
    }

    GoDestroy(dataset);

    return kOK;
  }

  kStatus GocatorReader::OnData(GoSensor sensor, GoDataSet dataset)
  {
    (void)sensor;

    double frame = 0.0;
    k64s encoder = 0;
    double y = 0.0;
    k16s *profile = 0;
    kSize profileWidth = 0;
    double xResolution = 0.0;
    double zResolution = 0.0;
    double xOffset = 0.0;
    double zOffset = 0.0;

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
          encoder = goStamp->encoder;
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

      if(m_use_encoder) {
        m_yOffset = encoder;
      }else {
        m_yOffset = frame;
      }
      spdlog::get("gocator")->info("First frame recieved from gocator. y :{}, x : {}, z : {}, zoff : {}, enc : {}, 1st : {}",m_yResolution, xResolution, zResolution, zOffset, m_encoder_resolution,m_yOffset);
      m_gocatorFifo.enqueue({msgid::resolutions, resolutions_t{m_yResolution, xResolution, zResolution, zOffset, m_encoder_resolution}});
    }

    if (m_use_encoder)
    {
      y = std::abs((encoder - m_yOffset) * m_encoder_resolution);
    }
    else
    {
      y = (frame - m_yOffset) * m_yResolution;
    }

    // Trim invalid values
    k16s *profile_end = profile + profileWidth - 1;
    for (; profile_end >= profile && *profile_end == k16S_NULL; --profile_end)
    {
    }
    profile_end++;

    auto profile_begin = profile;
    for (; profile_begin < profile_end && *profile_begin == k16S_NULL; ++profile_begin)
    {
    }

    auto samples_width = (double)distance(profile, profile_begin);
    auto z = GocatorReaderBase::k16sToFloat(profile_begin, profile_end, zResolution, zOffset);
    m_gocatorFifo.enqueue({msgid::scan, cads::profile{y, xOffset + samples_width * xResolution, std::move(z)}});


    return kOK;
  }

}
