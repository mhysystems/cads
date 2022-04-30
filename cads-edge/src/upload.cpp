#include "upload.h"
#include "db.h"

#include <cpr/cpr.h>
#include <z_data_generated.h>

#include <nlohmann/json.hpp>

#include <iostream>
#include <chrono>
#include <thread>
#include <limits>
#include <queue>


#include <date/date.h>
#include <date/tz.h>
#include <fmt/core.h>
#include <fmt/chrono.h>

#include <spdlog/spdlog.h>
#include <spdlog/sinks/rotating_file_sink.h>

using json = nlohmann::json;
extern json global_config;

using namespace std;

namespace cads 
{

void http_post_profile_properties(double y_resolution, double x_resolution, double z_resolution, double z_offset){
		json resolution;
	
		resolution["y"] = y_resolution;
		resolution["x"] = x_resolution;
		resolution["z"] = z_resolution;
		resolution["z_off"] = z_offset;

    http_post_profile_properties(resolution.dump());
}

void http_post_profile_properties(std::string json) {
  
  cpr::Response r;
  const cpr::Url endpoint{global_config["upload_config_to"].get<std::string>()};
    
  while(true) {
    r = cpr::Post(endpoint,
    cpr::Body{json},
    cpr::Header{{"Content-Type", "application/json"}});

    if(cpr::ErrorCode::OK == r.error.code && cpr::status::HTTP_OK == r.status_code) {
      break;
    }else {
      //log->error("First Upload failed with http status code {}", r.status_code);
    }
  }

}



void http_post_thread(moodycamel::BlockingReaderWriterQueue<uint64_t> &upload_fifo) {
	using namespace flatbuffers;
	
	sqlite3 *db = nullptr;
	const cpr::Url endpoint{global_config["upload_profile_to"].get<std::string>()};
	const char *db_name = global_config["db_name"].get<std::string>().c_str();

	int err = sqlite3_open_v2(db_name, &db, SQLITE_OPEN_READONLY | SQLITE_OPEN_NOMUTEX | SQLITE_OPEN_SHAREDCACHE, nullptr);
	auto stmt = fetch_profile_statement(db);

  std::queue<uint64_t> wait;

	auto log = spdlog::rotating_logger_st("upload", "upload.log", 1024 * 1024 * 5, 1);
	
  auto start = std::chrono::high_resolution_clock::now();
  while (true)
	{
    uint64_t msg; 
    uint64_t y = 0;
		
    if(!upload_fifo.wait_dequeue_timed(msg,std::chrono::seconds(1))) {
      if(wait.size() > 0) {
        y = wait.front();
        wait.pop();
      }else {
        upload_fifo.wait_dequeue(msg);
        y = msg;
      }

    }else {
      y = msg;
    }


		log->flush();
		
		cpr::Response r;

    auto p = fetch_profile(stmt,y) ; 
    
    // profile at y not written to database yet
    if(p.y != y) {
      wait.push(y);
      continue;
    }

    FlatBufferBuilder builder(4096);
    auto flat_buffer = cads_flatworld::CreateprofileDirect(builder,p.y,p.x_off,&p.z);
    builder.Finish(flat_buffer);

    auto buf = builder.GetBufferPointer();
    auto size = builder.GetSize();
    auto start = std::chrono::high_resolution_clock::now();
    while(true) {
      r = cpr::Post(endpoint,
              cpr::Body{(char *)buf,size},
              cpr::Header{{"Content-Type", "application/octet-stream"}});
      
      if(cpr::ErrorCode::OK == r.error.code && cpr::status::HTTP_OK == r.status_code) {
        break;
      }else {
        log->error("Upload failed with http status code {}", r.status_code);
      }
    }
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start).count() / 1000.0;

    log->info("SIZE: {}, DUR:{}, RATE(Kb/s):{} ",size, duration, (double)size / duration);
	}

	close_db(db,stmt);

}


void http_post_thread_bulk(moodycamel::BlockingReaderWriterQueue<uint64_t> &upload_fifo) {
	using namespace flatbuffers;
	
	sqlite3 *db = nullptr;

	auto g = fmt::format("{:%F-%H-%M}",std::chrono::system_clock::now());
  const cpr::Url endpoint{global_config["upload_profile_to"].get<std::string>()};

  auto dd = endpoint.str();
	const char *db_name = global_config["db_name"].get<std::string>().c_str();

	int err = sqlite3_open_v2(db_name, &db, SQLITE_OPEN_READONLY | SQLITE_OPEN_NOMUTEX | SQLITE_OPEN_SHAREDCACHE, nullptr);
	auto stmt = fetch_profile_statement(db);

  std::queue<uint64_t> wait;

	auto log = spdlog::rotating_logger_st("upload", "upload.log", 1024 * 1024 * 5, 1);
	
  auto start = std::chrono::high_resolution_clock::now();
  auto cnt = 128;
  FlatBufferBuilder builder(4096 * 128);
  std::vector<flatbuffers::Offset<cads_flatworld::profile>> profiles_flat;
  
  while (true)
	{
    uint64_t y = 0;
		
    if(!upload_fifo.try_dequeue(y)) {
      if(wait.size() > 0) {
        y = wait.front();
        wait.pop();
      }else {
        upload_fifo.wait_dequeue(y);
      }
    }

    if(y == std::numeric_limits<uint64_t>::max() && wait.size() == 0) break;
    if(y == std::numeric_limits<uint64_t>::max()) continue;


		log->flush();
		
    auto p = fetch_profile(stmt,y) ; 
    
    // profile at y not written to database yet
    if(p.y != y) {
      wait.push(y);
      continue;
    }

    profiles_flat.push_back(cads_flatworld::CreateprofileDirect(builder,p.y,p.x_off,&p.z));
    if(cnt-- > 0) continue;

    cnt = 128;
    builder.Finish(cads_flatworld::Createprofile_array(builder,128,builder.CreateVector(profiles_flat)));
    

    auto buf = builder.GetBufferPointer();
    auto size = builder.GetSize();
    auto start = std::chrono::high_resolution_clock::now();
    
    while(true) {
     cpr::Response r = cpr::Post(endpoint,
              cpr::Body{(char *)buf,size},
              cpr::Header{{"Content-Type", "application/octet-stream"}});
      
      if(cpr::ErrorCode::OK == r.error.code && cpr::status::HTTP_OK == r.status_code) {
        break;
      }else {
        log->error("Upload failed with http status code {}", r.status_code);
      }
    }
    
    builder.Clear();
    profiles_flat.clear();

    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start).count() / 1000.0;

    log->info("SIZE: {}, DUR:{}, RATE(Kb/s):{} ",size, duration, (double)size / duration);
	}

	close_db(db,stmt);

}

}
