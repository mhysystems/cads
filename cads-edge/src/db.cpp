#include "db.h"
#include "json.hpp"

#include <vector>
#include <string>
#include <iostream>

#include <readerwriterqueue.h>
#include <spdlog/spdlog.h>
#include <spdlog/sinks/rotating_file_sink.h>

using namespace moodycamel;
using json = nlohmann::json;
extern json global_config;

namespace cads {

using namespace std;

bool create_db(std::string name) {
    sqlite3 *db = nullptr;
    bool r = false;
    const char *db_name = name.c_str();
    int err = sqlite3_open_v2(db_name, &db, SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE , nullptr);
    vector<string> tables{
        R"(CREATE TABLE IF NOT EXISTS PROFILE (y INTEGER PRIMARY KEY, x_off REAL NOT NULL, z BLOB NOT NULL);)"s,
        R"(CREATE TABLE IF NOT EXISTS PARAMETERS (y_res REAL NOT NULL, x_res REAL NOT NULL, z_res REAL NOT NULL, z_off REAL NOT NULL);)"s
//        R"(CREATE TABLE IF NOT EXISTS BELT (index INTEGER PRIMARY KEY AUTOINCREMENT, num_y_samples integer,num_x_samples integer,belt_length real,x_start real,x_end real,z_min real, z_max real ))"s,
//        R"(CREATE TABLE IF NOT EXISTS GUI (anomaly_ID integer not null primary key, visible integer ))"s,
//        R"(CREATE TABLE IF NOT EXISTS A_TRACKING (anomaly_ID integer not null primary key, start real, length real, x_lower real, x_upper real, z_depth real, area real,volume real, time text, epoch integer, contour_x real, contour_theta real, category text, danger integer,comment text))"s
    };


    if(err == SQLITE_OK){
        
        sqlite3_stmt *stmt = nullptr;
        for(auto query : tables) {
        
            err = sqlite3_prepare_v2(db, query.c_str(), query.size(), &stmt, NULL);
            if(err == SQLITE_OK) { 

                err = sqlite3_step(stmt);
                r = true;
            }
        }
        
        if(stmt != nullptr) sqlite3_finalize(stmt);
        
    }

    if(db != nullptr) sqlite3_close(db);

    return r;
}


tuple<sqlite3 *,sqlite3_stmt*> open_db(std::string name) {
    sqlite3 *db = nullptr;
    const char *db_name = name.c_str();

    int err = sqlite3_open_v2(db_name, &db, SQLITE_OPEN_READWRITE | SQLITE_OPEN_NOMUTEX | SQLITE_OPEN_SHAREDCACHE, nullptr);
        
    sqlite3_stmt *stmt = nullptr;
    //auto query = R"(BEGIN TRANSACTION)"s;
    auto query = R"(PRAGMA synchronous=OFF)"s;
    err = sqlite3_prepare_v2(db, query.c_str(), query.size(), &stmt, NULL);
    err = sqlite3_step(stmt);
    query = R"("PRAGMA locking_mode = EXCLUSIVE")"s;
    err = sqlite3_prepare_v2(db, query.c_str(), query.size(), &stmt, NULL);
    err = sqlite3_step(stmt);
		query = R"("PRAGMA journal_mode = MEMORY")"s;
    err = sqlite3_prepare_v2(db, query.c_str(), query.size(), &stmt, NULL);
    err = sqlite3_step(stmt);
    
    query = R"(INSERT OR REPLACE INTO PROFILE (y,x_off,z) VALUES (?,?,?))"s;
    err = sqlite3_prepare_v2(db, query.c_str(), query.size(), &stmt, NULL);

    return {db,stmt};

}


sqlite3_stmt* fetch_profile_statement(sqlite3* db) {
    auto query = R"(SELECT * FROM PROFILE WHERE y=?)"s;
    sqlite3_stmt *stmt = nullptr;
    auto err = sqlite3_prepare_v2(db, query.c_str(), query.size(), &stmt, NULL); 
    return stmt;
}

profile fetch_profile(sqlite3_stmt* stmt, uint64_t y) {
    int err = sqlite3_bind_int64(stmt,1,y);
		err = sqlite3_step(stmt);
    
    if( err == SQLITE_ROW){
        uint64_t y = (uint64_t)sqlite3_column_int64(stmt,0);
        double x_off = sqlite3_column_double(stmt,1);
        int16_t *z = (int16_t*)sqlite3_column_blob(stmt,2);
        int len = sqlite3_column_bytes(stmt,2) / sizeof(*z);
				vector<int16_t> zv{z,z+len};
       // err = sqlite3_step(stmt);
				sqlite3_reset(stmt);
        return {y,x_off,zv};
    }else {
        sqlite3_reset(stmt);
        return {std::numeric_limits<uint64_t>::max(),NAN,{}};
    }
}



std::tuple<double,double,double,double> fetch_profile_parameters(std::string name) {
    
  sqlite3 *db = nullptr;
  sqlite3_stmt *stmt = nullptr;

  const char *db_name = name.c_str();
  int err = sqlite3_open_v2(db_name, &db, SQLITE_OPEN_READONLY, nullptr);
  
  auto query = R"(SELECT * FROM PARAMETERS)"s;
  err = sqlite3_prepare_v2(db, query.c_str(), query.size(), &stmt, NULL); 
  
  err = sqlite3_step(stmt);
  
  std::tuple<double,double,double,double> rtn;
  if( err == SQLITE_ROW){

    rtn = {
      sqlite3_column_double(stmt,0),
      sqlite3_column_double(stmt,1),
      sqlite3_column_double(stmt,2),
      sqlite3_column_double(stmt,3)
    };

  } else {
    rtn = {1.0,1.0,1.0,1.0};
  }


  if(stmt != nullptr) sqlite3_finalize(stmt);
  if(db != nullptr) sqlite3_close(db);

  return rtn;

}

void store_profile_parameters(double y_res, double x_res, double z_res, double z_off) 
{
  sqlite3 *db = nullptr;
  sqlite3_stmt *stmt = nullptr;

  const char *db_name = global_config["db_name"].get<std::string>().c_str();
  int err = sqlite3_open_v2(db_name, &db, SQLITE_OPEN_READWRITE, nullptr);
  
  auto query = R"(INSERT OR REPLACE INTO PARAMETERS (y_res,x_res,z_res,z_off) VALUES (?,?,?,?))"s;
  err = sqlite3_prepare_v2(db, query.c_str(), query.size(), &stmt, NULL);
  err = sqlite3_bind_double(stmt,1,y_res);
  err = sqlite3_bind_double(stmt,2,x_res);
  err = sqlite3_bind_double(stmt,3,z_res);
  err = sqlite3_bind_double(stmt,4,z_off);

  err = SQLITE_BUSY;

  while(err == SQLITE_BUSY) {
    err = sqlite3_step(stmt);
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
  }

  if(stmt != nullptr) sqlite3_finalize(stmt);
  if(db != nullptr) sqlite3_close(db);

}


coop::task_t<void,true> store_profile_thread(BlockingReaderWriterQueue<profile> &db_fifo) {
	  
    co_await coop::suspend();

		auto log = spdlog::rotating_logger_st("db", "db.log", 1024 * 1024 * 5, 1);
		
		sqlite3 *db = nullptr;
    const char *db_name = global_config["db_name"].get<std::string>().c_str();

    int err = sqlite3_open_v2(db_name, &db, SQLITE_OPEN_READWRITE | SQLITE_OPEN_NOMUTEX | SQLITE_OPEN_SHAREDCACHE, nullptr);
        
    sqlite3_stmt *stmt = nullptr;
		auto query = R"(PRAGMA synchronous=OFF)"s;
    err = sqlite3_prepare_v2(db, query.c_str(), query.size(), &stmt, NULL);
    err = sqlite3_step(stmt);
    query = R"("PRAGMA locking_mode = EXCLUSIVE")"s;
    err = sqlite3_prepare_v2(db, query.c_str(), query.size(), &stmt, NULL);
    err = sqlite3_step(stmt);
		query = R"("PRAGMA journal_mode = MEMORY")"s;
    err = sqlite3_prepare_v2(db, query.c_str(), query.size(), &stmt, NULL);
    err = sqlite3_step(stmt);
		query = R"("PRAGMA temp_store = MEMORY")"s;
    err = sqlite3_prepare_v2(db, query.c_str(), query.size(), &stmt, NULL);
    err = sqlite3_step(stmt);
    query = R"("PRAGMA mmap_size = 10000000")"s;
    err = sqlite3_prepare_v2(db, query.c_str(), query.size(), &stmt, NULL);
    err = sqlite3_step(stmt);
    query = R"("PRAGMA page_size = 32768")"s;
    err = sqlite3_prepare_v2(db, query.c_str(), query.size(), &stmt, NULL);
    err = sqlite3_step(stmt);
    query = R"(INSERT OR REPLACE INTO PROFILE (y,x_off,z) VALUES (?,?,?))"s;
    err = sqlite3_prepare_v2(db, query.c_str(), query.size(), &stmt, NULL);
		
		while(true) {
			
			log->flush();
      profile p;
      db_fifo.wait_dequeue(p);
      
      if(p.y == std::numeric_limits<uint64_t>::max()) break;

			err = sqlite3_bind_int64(stmt,1,(int64_t)p.y);
			err = sqlite3_bind_double(stmt,2,p.x_off);
			err = sqlite3_bind_blob(stmt, 3, p.z.data(), p.z.size()*sizeof(int16_t), SQLITE_STATIC );

			err = SQLITE_BUSY;

			while(err == SQLITE_BUSY) {
				err = sqlite3_step(stmt);
				//std::this_thread::sleep_for(std::chrono::milliseconds(50));
			}
			
      //sqlite3_clear_bindings(stmt);
			sqlite3_reset(stmt);  

			//if(err == SQLITE_DONE) { 
			//	log->info("Stored {} in DB after initial failue. Removed from queue.",p.y); 
			//}
			//else{ 
				//log->info("Store {} failed with err {} . If err is 6 DB is just temporary locked.",p.y, err); 
			//}
      

		}

		if(stmt != nullptr) sqlite3_finalize(stmt);
    if(db != nullptr) sqlite3_close(db);

}

bool store_profile(sqlite3_stmt* stmt, const profile& p) {
			int err = sqlite3_bind_int64(stmt,1,(int64_t)p.y); 
      err = sqlite3_bind_double(stmt,2,p.x_off);
			err = sqlite3_bind_blob(stmt, 3, p.z.data(), p.z.size()*sizeof(int16_t), SQLITE_STATIC );
    //SQLITE_DONE 
    
		err = sqlite3_step(stmt);

	  sqlite3_reset(stmt);  

		return err == SQLITE_DONE;
}

void close_db(sqlite3 *db, sqlite3_stmt* stmt,sqlite3_stmt* stmt2) {

        
    if(stmt != nullptr) sqlite3_finalize(stmt);
    if(stmt2 != nullptr) sqlite3_finalize(stmt2);
    if(db != nullptr) sqlite3_close(db);

}

}
