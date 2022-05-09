#include "gocator_reader.h"
#include <z_data_generated.h>
#include <p_config_generated.h>


#include <GoSdk/GoSdk.h>

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

template<class T>
decltype(auto) nmToMm(T in)
{
    return in / 1e6;
}

template<class T>
decltype(auto) umToMm(T in)
{
    return in / 1e3;
}

const k16s InvalidRange16Bit = 0x8000;

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
    }
}

void GocatorReader::Stop()
{
    auto status = GoSensor_Stop(m_sensor);

    if (kIsError(status))
    {
        throw runtime_error{"GoSensor_Stop: "s + to_string(status)};
    }
}


GocatorReader::GocatorReader(moodycamel::BlockingReaderWriterQueue<profile>& gocatorFifo) : 
	GocatorReaderBase(gocatorFifo)
{
	m_assembly = CreateGoSdk();
	m_system = CreateGoSystem();
  first_frame_offset = 0;
	
	if (GoSystem_SensorCount(m_system) < 1)
	{
			throw runtime_error{"GoSystem_SensorCount: No sensor found"};
	}
	
	m_sensor = GoSystem_SensorAt(m_system, 0);
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

	// :TODO: get the actual value, not the configured one
	m_yResolution = 0.1221; //GoSetup_EncoderSpacing(setup);

}

kStatus kCall GocatorReader::OnData(kPointer context, GoSensor sensor, GoDataSet dataset)
{
    return static_cast<GocatorReader*>(context)->OnData(sensor, dataset);
}

kStatus kCall GocatorReader::OnSystem(kPointer context, GoSystem system, GoDataSet data){
  return static_cast<GocatorReader*>(context)->OnSystem(system, data);
}


GocatorReader::~GocatorReader() {
	m_loop = false;
	Stop();
	GoDestroy(m_system);
	GoDestroy(m_assembly);
}

void GocatorReader::RunForever()
{
	Start();
	spdlog::info("Started Gocator");
	
	while(m_loop) {
		unique_lock<mutex> lock(m_mutex);
		m_condition.wait(lock);
	}

	Stop();
	spdlog::info("Stopped Gocator");
}

kStatus GocatorReader::OnSystem(GoSystem system, GoDataSet dataset)
{
    for (auto i = 0; i < GoDataSet_Count(dataset); ++i)
    {
        GoHealthMsg message = GoDataSet_At(dataset, i);
        for (auto k = 0; k < GoHealthMsg_Count(message); k++)
        {
          auto healthIndicator = GoHealthMsg_At(message, k);
          if(healthIndicator->id == GO_HEALTH_ENCODER_VALUE)
            spdlog::info("Indicator[{}]: Id:{} Instance:{} Value:{}", k, healthIndicator->id, healthIndicator->instance, healthIndicator->value); 
        }
    }


    GoDestroy(dataset);


    return kOK;
}


kStatus GocatorReader::OnData(GoSensor sensor, GoDataSet dataset)
{
    k64u frame = 0;
    k64s encoder = 0;
    double y = 0.0;
    k16s* profile = 0;
    kSize profileWidth = 0;
    double xResolution = 0.0;
    double zResolution = 0.0;
    double xOffset = 0.0;
    double zOffset = 0.0;

    for (auto i = 0; i < GoDataSet_Count(dataset); ++i)
    {
        auto message = GoDataSet_At(dataset, i);

        switch (GoDataMsg_Type(message))
        {
          case GO_DATA_MESSAGE_TYPE_STAMP:
            {
                for (auto j = 0; j < GoStampMsg_Count(message); ++j)
                {
                    GoStamp *goStamp = GoStampMsg_At(message, j);
                    frame = goStamp->frameIndex;
                    frame = (k64u)goStamp->encoder;
                    //y = frame * m_yResolution;
                    y = encoder * 0.1221;
                }
            }
            break;
          case GO_DATA_MESSAGE_TYPE_UNIFORM_PROFILE:
            {
								auto num = GoUniformProfileMsg_Count(message); 
                for (auto j = 0; j < num; ++j)
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

		if(m_first_frame) {
      m_first_frame = false;
			spdlog::info("First frame recieved from gocator");
      first_frame_offset = frame;
      m_xResolution = xResolution;
      m_zResolution = zResolution;
      m_zOffset = zOffset;

		}

		// Trim invalid values
		k16s* profile_end = profile + profileWidth - 1;
		for (; profile_end >= profile && *profile_end == InvalidRange16Bit ;--profile_end){}
		profile_end++;

		auto profile_begin = profile;
		for (; profile_begin < profile_end && *profile_begin == InvalidRange16Bit ;++profile_begin){}

		auto samples_width = distance(profile,profile_begin);

	  m_gocatorFifo.enqueue({frame-first_frame_offset,xOffset+samples_width*xResolution,{profile_begin,profile_end}});		

    GoDestroy(dataset);

		{
			unique_lock<mutex> lock(m_mutex);
			m_condition.notify_all();
    }

    return kOK;
}

}
