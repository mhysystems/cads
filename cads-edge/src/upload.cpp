#include "upload.h"
#include "db.h"

#include <cpr/cpr.h>
#include <z_data_generated.h>

#include "json.hpp"

#include <iostream>
#include <chrono>
#include <thread>
#include <limits>

#include <fcntl.h>
#include <unistd.h>
#include <errno.h>

#include <spdlog/spdlog.h>
#include <spdlog/sinks/rotating_file_sink.h>



using json = nlohmann::json;
extern json global_config;

using namespace std;

namespace cads 
{


void http_post_thread(std::queue<std::variant<uint64_t,std::string>> &q, std::mutex &m, std::condition_variable &sig) {
	using namespace flatbuffers;
	
	sqlite3 *db = nullptr;
	const cpr::Url endpoint{global_config["upload_profile_to"].get<std::string>()};
	const char *db_name = "profile.db"; //global_config["db_name"].get<std::string>().c_str();
	const long up_limit = global_config["daily_upload_limit"].get<long>();
	long up_daily = 0;

	int err = sqlite3_open_v2(db_name, &db, SQLITE_OPEN_READONLY | SQLITE_OPEN_NOMUTEX | SQLITE_OPEN_SHAREDCACHE, nullptr);
	auto stmt = fetch_profile_statement(db);

	int open_error = errno;

	auto log = spdlog::rotating_logger_st("upload", "upload.log", 1024 * 1024 * 5, 1);
	
	bool one_shot = true;
	
	while (true)
	{
		unique_lock<mutex> lock(m);
		if(q.empty())  {  
			one_shot = true;  
			sig.wait(lock,[&](){ return !q.empty();});
		}
		
		log->flush();
		const auto msg = q.front();
		const auto q_size = q.size();
		lock.unlock();
		
		cpr::Response r;
		if(msg.index() == 1) {
				const cpr::Url endpoint{global_config["upload_config_to"].get<std::string>()};
				auto json = get<string>(msg);
				r = cpr::Post(endpoint,
				cpr::Body{json},
				cpr::Header{{"Content-Type", "application/json"}});

		}else {
			auto y = get<uint64_t>(msg);

			auto p = fetch_profile(stmt,y) ; 
      log->info("fetch:{},{}",p.z[0],p.z.size());
			// profile at y not written to database yet
			if(p.y != y) {
				unique_lock<mutex> lock(m);
				q.pop();
				q.push(msg);
				continue;
			} 

			FlatBufferBuilder builder(4096);
			auto flat_buffer = cads_flatworld::CreateprofileDirect(builder,p.y,p.x_off,&p.z);
	
			builder.Finish(flat_buffer);

			auto buf = builder.GetBufferPointer();
			auto size = builder.GetSize();
			
			r = cpr::Post(endpoint,
							cpr::Body{(char *)buf,size},
							cpr::Header{{"Content-Type", "application/octet-stream"}});
			
			up_daily += size;   
			if(one_shot && up_daily+q_size*size> up_limit) {
				log->info("Daily limit of {} reached. Sending sleep to recorder.",up_limit); 
				up_daily = 0;
				one_shot = false;
			}

		}
			
		if(cpr::ErrorCode::OK == r.error.code && cpr::status::HTTP_OK == r.status_code) {
			unique_lock<mutex> lock(m);
			q.pop();
		}else {
			log->error("Upload failed with http status code {}", r.status_code);
		}
	}
	
	close_db(db,stmt);
}

}
