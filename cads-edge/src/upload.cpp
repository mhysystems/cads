#include "upload.h"
#include "db.h"

#include <cpr/cpr.h>
#include <z_data_generated.h>

#include "json.hpp"

#include <iostream>
#include <chrono>
#include <thread>
#include <limits>
#include <queue>

#include <spdlog/spdlog.h>
#include <spdlog/sinks/rotating_file_sink.h>



using json = nlohmann::json;
extern json global_config;

using namespace std;

namespace cads 
{


void http_post_thread(moodycamel::BlockingReaderWriterQueue<std::variant<uint64_t,std::string>> &upload_fifo) {
	using namespace flatbuffers;
	
	sqlite3 *db = nullptr;
	const cpr::Url endpoint{global_config["upload_profile_to"].get<std::string>()};
	const char *db_name = global_config["db_name"].get<std::string>().c_str();

	int err = sqlite3_open_v2(db_name, &db, SQLITE_OPEN_READONLY | SQLITE_OPEN_NOMUTEX | SQLITE_OPEN_SHAREDCACHE, nullptr);
	auto stmt = fetch_profile_statement(db);

  std::queue<uint64_t> wait;

	auto log = spdlog::rotating_logger_st("upload", "upload.log", 1024 * 1024 * 5, 1);
	
  {
    cpr::Response r;
    std::variant<uint64_t,std::string> msg; 
    upload_fifo.wait_dequeue(msg);
    const cpr::Url endpoint{global_config["upload_config_to"].get<std::string>()};
    auto json = get<string>(msg);
    
    while(true) {
      r = cpr::Post(endpoint,
      cpr::Body{json},
      cpr::Header{{"Content-Type", "application/json"}});

      if(cpr::ErrorCode::OK == r.error.code && cpr::status::HTTP_OK == r.status_code) {
        break;
		  }else {
			  log->error("First Upload failed with http status code {}", r.status_code);
		  }
    }
  }
  
  
  while (true)
	{
    std::variant<uint64_t,std::string> msg; 
    uint64_t y = 0;
		
    if(!upload_fifo.wait_dequeue_timed(msg,std::chrono::seconds(1))) {
      if(wait.size() > 0) {
        y = wait.front();
        wait.pop();
      }else {
        upload_fifo.wait_dequeue(msg);
        y = get<uint64_t>(msg);
      }

    }else {
      y = get<uint64_t>(msg);
    }


		log->flush();
		
		cpr::Response r;

    auto p = fetch_profile(stmt,y) ; 
    log->info("fetch:{},{}",p.z[0],p.z.size());
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
	}

	close_db(db,stmt);

}

}
