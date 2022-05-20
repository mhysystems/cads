#pragma once

#include <tuple>
#include <sqlite3.h>
#include <cstdint>
#include <string>

#include <readerwriterqueue.h>

#include <window.hpp>
#include <profile.h>
#include <coro.hpp>

namespace cads{

bool create_db(std::string name);
std::tuple<sqlite3 *,sqlite3_stmt*> open_db(std::string name);
sqlite3_stmt* fetch_profile_statement(sqlite3*);
bool store_profile(sqlite3_stmt*, const profile&);
profile fetch_profile(sqlite3_stmt*, y_type);
void close_db(sqlite3 *db = nullptr, sqlite3_stmt* stmt = nullptr,sqlite3_stmt* stmt2 = nullptr); 
void store_profile_thread(moodycamel::BlockingReaderWriterQueue<cads::profile> &db_fifo);
coro<y_type,profile> store_profile_coro(profile p);
void store_profile_parameters(double y_res, double x_res, double z_res, double z_off, double encoder_res);
std::tuple<double,double,double,double,double> fetch_profile_parameters(std::string name);


}

