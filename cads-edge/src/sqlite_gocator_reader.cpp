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
#include <json.hpp>

using namespace std;
using namespace std::chrono;

using json = nlohmann::json;

extern json global_config;

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
  auto data_src = global_config["data_source"].get<std::string>();
  auto [m_yResolution,xResolution,zResolution,zOffset] = fetch_profile_parameters(data_src);

  flatbuffers::FlatBufferBuilder builder(4096);
  
  auto flat_buffer = cads_flatworld::Createprofile_resolution(builder,m_yResolution,xResolution,zResolution,zOffset);
  builder.Finish(flat_buffer);

  auto buf = builder.GetBufferPointer();
  auto size = builder.GetSize();

  m_gocatorFifo.enqueue((char*)&size,sizeof(size));
  m_gocatorFifo.enqueue((char*)buf,size);

  uint64_t y = 0;

  
  auto [db, stmt] = open_db(data_src);
  auto fetch_stmt = fetch_profile_statement(db);



    while(m_loop) {
		  cads::profile profile = fetch_profile(fetch_stmt, y++);

      if(profile.y == std::numeric_limits<uint64_t>::max()) continue;

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
