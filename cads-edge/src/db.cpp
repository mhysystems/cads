#include "db.h"

#include <vector>
#include <string>
#include <iostream>
#include "json.hpp"

#include <spdlog/spdlog.h>
#include <spdlog/sinks/rotating_file_sink.h>




using json = nlohmann::json;
extern json global_config;

namespace cads {

using namespace std;

bool create_db() {
    sqlite3 *db = nullptr;
    bool r = false;
    const char *db_name = global_config["db_name"].get<std::string>().c_str();
    int err = sqlite3_open_v2(db_name, &db, SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE , nullptr);
    vector<string> tables{
        R"(CREATE TABLE IF NOT EXISTS PROFILE (y INTEGER PRIMARY KEY, x_off REAL NOT NULL, left_edge REAL NOT NULL, right_edge REAL NOT NULL,z BLOB NOT NULL);)"s,
//        R"(CREATE TABLE IF NOT EXISTS BELT (index INTEGER PRIMARY KEY AUTOINCREMENT, num_y_samples integer,num_x_samples integer,belt_length real,x_start real,x_end real,z_min real, z_max real ))"s,
//        R"(CREATE TABLE IF NOT EXISTS GUI (anomaly_ID integer not null primary key, visible integer ))"s,
        R"(CREATE TABLE IF NOT EXISTS A_TRACKING (anomaly_ID integer not null primary key, start real, length real, x_lower real, x_upper real, z_depth real, area real,volume real, time text, epoch integer, contour_x real, contour_theta real, category text, danger integer,comment text))"s
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


tuple<sqlite3 *,sqlite3_stmt*> open_db() {
    sqlite3 *db = nullptr;
    const char *db_name = global_config["db_name"].get<std::string>().c_str();

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

    query = R"(INSERT OR REPLACE INTO PROFILE (y,x_off,left_edge,right_edge,z) VALUES (?,?,?,?,?))"s;
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
        double left_edge = sqlite3_column_double(stmt,2);
        double right_edge = sqlite3_column_double(stmt,3);
        int16_t *z = (int16_t*)sqlite3_column_blob(stmt,4);
        int len = sqlite3_column_bytes(stmt,4) / sizeof(*z);
				vector<int16_t> zv{z,z+len};
       // err = sqlite3_step(stmt);
				sqlite3_reset(stmt);
        return {y,x_off,left_edge,right_edge,zv};
    }else {
        sqlite3_reset(stmt);
        return {std::numeric_limits<uint64_t>::max(),NAN,NAN,NAN,{}};
    }


}

void store_profile_thread(std::queue<profile> &q, std::mutex &m, std::condition_variable &sig) {
	  
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
    query = R"(INSERT OR REPLACE INTO PROFILE (y,x_off,left_edge,right_edge,z) VALUES (?,?,?,?,?))"s;
    err = sqlite3_prepare_v2(db, query.c_str(), query.size(), &stmt, NULL);
		
		while(true) {
			unique_lock<mutex> lock(m);
			
			if(q.empty()) {            
				sig.wait(lock,[&](){ return !q.empty();});
			}
			
			log->flush();
			const auto p = q.front(); 
			lock.unlock();

			err = sqlite3_bind_int64(stmt,1,(int64_t)p.y);
			err = sqlite3_bind_double(stmt,2,p.x_off);
      err = sqlite3_bind_double(stmt,3,p.left_edge);
      err = sqlite3_bind_double(stmt,4,p.right_edge);
			err = sqlite3_bind_blob(stmt, 5, p.z.data(), p.z.size()*sizeof(int16_t), SQLITE_STATIC );

			err = SQLITE_BUSY;

			while(err == SQLITE_BUSY) {
				err = sqlite3_step(stmt);
				std::this_thread::sleep_for(std::chrono::milliseconds(50));
			}
			sqlite3_clear_bindings(stmt);
			sqlite3_reset(stmt);  

			lock.lock();
			if(err == SQLITE_DONE) { 
				q.pop();
				log->info("Stored {} in DB after initial failue. Removed from queue.",p.y); 
			}
			else{ 
				log->info("Store {} failed with err {} . If err is 6 DB is just temporary locked.",p.y, err); 
			}

		}

		if(stmt != nullptr) sqlite3_finalize(stmt);
    if(db != nullptr) sqlite3_close(db);

}


bool store_profile(sqlite3_stmt* stmt, profile p) {
			int err = sqlite3_bind_int64(stmt,1,(int64_t)p.y); 
      err = sqlite3_bind_double(stmt,2,p.x_off);
      err = sqlite3_bind_double(stmt,3,p.left_edge);
      err = sqlite3_bind_double(stmt,4,p.right_edge);
			err = sqlite3_bind_blob(stmt, 5, p.z.data(), p.z.size()*sizeof(int16_t), SQLITE_STATIC );
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
