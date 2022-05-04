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
#include <unordered_map>
#include <random>


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

std::string ReplaceString(std::string subject, const std::string& search,
                          const std::string& replace) {
    size_t pos = 0;
    while((pos = subject.find(search, pos)) != std::string::npos) {
         subject.replace(pos, search.length(), replace);
         pos += replace.length();
    }
    return subject;
}


void send_flatbuffer_array(
  flatbuffers::FlatBufferBuilder &builder, 
  std::vector<flatbuffers::Offset<cads_flatworld::profile>> &profiles_flat,
  const cpr::Url& endpoint,
  std::shared_ptr<spdlog::logger> log
  ) {
    
    builder.Finish(cads_flatworld::Createprofile_array(builder,profiles_flat.size(),builder.CreateVector(profiles_flat)));

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

void http_post_profile_properties(double y_resolution, double x_resolution, double z_resolution, double z_offset, std::string ts){
		json resolution;
	
		resolution["y"] = y_resolution;
		resolution["x"] = x_resolution;
		resolution["z"] = z_resolution;
		resolution["z_off"] = z_offset;

    http_post_profile_properties(resolution.dump(), ts);
}

void http_post_profile_properties(std::string json, std::string ts) {
  
  auto endpoint_url = global_config["upload_profile_to"].get<std::string>();
  std::transform(endpoint_url.begin(), endpoint_url.end(), endpoint_url.begin(),[](unsigned char c){ return std::tolower(c); });
  if(endpoint_url == "null") return;
  
  cpr::Response r;
  const cpr::Url endpoint{ReplaceString(global_config["upload_config_to"].get<std::string>(),"%DATETIME%"s,ts)};
    
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
	const cpr::Url endpoint{ReplaceString(global_config["upload_profile_to"].get<std::string>(),"%DATETIME%"s,"-hello"s)};
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


void http_post_thread_bulk(moodycamel::BlockingReaderWriterQueue<uint64_t> &upload_fifo, std::string ts) {
	using namespace flatbuffers;
	
	sqlite3 *db = nullptr;
  auto endpoint_url = global_config["upload_profile_to"].get<std::string>();
  std::transform(endpoint_url.begin(), endpoint_url.end(), endpoint_url.begin(),[](unsigned char c){ return std::tolower(c); });
  if(endpoint_url == "null") return;
  
  const cpr::Url endpoint{ReplaceString(global_config["upload_profile_to"].get<std::string>(),"%DATETIME%"s,ts)};

	const char *db_name = global_config["db_name"].get<std::string>().c_str();

	int err = sqlite3_open_v2(db_name, &db, SQLITE_OPEN_READONLY | SQLITE_OPEN_NOMUTEX | SQLITE_OPEN_SHAREDCACHE, nullptr);
	auto stmt = fetch_profile_statement(db);

  std::unordered_map<uint64_t,int> wait{};

	auto log = spdlog::rotating_logger_st("upload", "upload.log", 1024 * 1024 * 5, 1);
	
  auto start = std::chrono::high_resolution_clock::now();

  FlatBufferBuilder builder(4096 * 128);
  std::vector<flatbuffers::Offset<cads_flatworld::profile>> profiles_flat;
  bool EOB = false;

  std::default_random_engine gen;
  while (!EOB || wait.size() > 0)
	{	
    log->flush();
    uint64_t y = 0;
		
    if(!EOB && !upload_fifo.try_dequeue(y)) {
      if(wait.size() > 0) {
        y = wait.begin()->second;
      }else {
        upload_fifo.wait_dequeue(y);
      }
    }

    if(y == std::numeric_limits<uint64_t>::max()) EOB = true;

    if(EOB && wait.size() > 0) {
      
      std::uniform_int_distribution<int> dist{0, wait.size()-1};
      auto b = dist(gen);
      y = wait.begin()->second;
    }
    
    auto p = fetch_profile(stmt,y) ; 
    
    // profile at y not written to database yet
    if(p.y == std::numeric_limits<uint64_t>::max()) {
      if(y != std::numeric_limits<uint64_t>::max()) {
        auto num = wait.count(y);
        if( num == 0) {
          wait.insert({y,1});
        }else if(num > 1024) {
          wait.erase(y);
        }else {
          wait[y]++;
        }
      }
    }else {
      profiles_flat.push_back(cads_flatworld::CreateprofileDirect(builder,p.y,p.x_off,&p.z));
    }

    if(profiles_flat.size() == 128 || (EOB && wait.size() == 0 && profiles_flat.size() > 0)) {
      send_flatbuffer_array(builder,profiles_flat,endpoint,log);
    }
    
	}

	close_db(db,stmt);

}



}
