#include <sqlite_gocator_reader.h>
#include <z_data_generated.h>
#include <p_config_generated.h>


#include <algorithm>
#include <ios>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <thread>
#include <vector>

#include <spdlog/spdlog.h>
#include <db.h>

using namespace std;
using namespace std::chrono;

namespace cads
{

void SqliteGocatorReader::Start()
{
  if(!m_loop) {
    m_loop = true;
    m_thread = std::thread{&SqliteGocatorReader::OnData, this};
  }
}

void SqliteGocatorReader::Stop()
{
  if(m_loop) {
    m_loop = false;
    m_thread.join();
  }
}


SqliteGocatorReader::SqliteGocatorReader(moodycamel::BlockingReaderWriterQueue<char>& gocatorFifo) : 
	GocatorReaderBase(gocatorFifo)
{}
	


SqliteGocatorReader::~SqliteGocatorReader() {

}

void SqliteGocatorReader::RunForever()
{
}

void SqliteGocatorReader::OnData()
{
    double xResolution = 0.0;
    double zResolution = 0.0;
    double xOffset = 0.0;
    double zOffset = 0.0;


    spdlog::info("First frame recieved from gocator");

    flatbuffers::FlatBufferBuilder builder(4096);
    
    auto flat_buffer = cads_flatworld::Createprofile_resolution(builder,m_yResolution,xResolution,zResolution,zOffset);
    builder.Finish(flat_buffer);

    auto buf = builder.GetBufferPointer();
    auto size = builder.GetSize();

    m_gocatorFifo.enqueue((char*)&size,sizeof(size));
    m_gocatorFifo.enqueue((char*)buf,size);

    uint64_t y = 0;

    while(m_loop) {
		  cads::profile profile = fetch_profile(nullptr, y++);

      flatbuffers::FlatBufferBuilder builder(4096);
      
      auto flat_buffer = cads_flatworld::CreateprofileDirect(builder,profile.y,profile.x_off,&profile.z);
      builder.Finish(flat_buffer);

      auto buf = builder.GetBufferPointer();
      auto size = builder.GetSize();

      m_gocatorFifo.enqueue((char*)&size,sizeof(size));
      m_gocatorFifo.enqueue((char*)buf,size);	
    }	

}

}
