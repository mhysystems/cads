#ifndef CADS_DB
#define CADS_DB 1

#include <tuple>
#include <sqlite3.h>
#include <cstdint>
#include <string>

#include <readerwriterqueue.h>

#include "cads.h"
#include <window.hpp>

namespace cads{

bool create_db(std::string name);
std::tuple<sqlite3 *,sqlite3_stmt*> open_db(std::string name);
sqlite3_stmt* fetch_profile_statement(sqlite3*);
bool store_profile(sqlite3_stmt*, const profile&);
profile fetch_profile(sqlite3_stmt*, uint64_t);
void close_db(sqlite3 *db = nullptr, sqlite3_stmt* stmt = nullptr,sqlite3_stmt* stmt2 = nullptr); 
void store_profile_thread(moodycamel::BlockingReaderWriterQueue<cads::profile> &db_fifo);
void store_profile_parameters(double y_res, double x_res, double z_res, double z_off);
std::tuple<double,double,double,double> fetch_profile_parameters(std::string name);


}

#endif