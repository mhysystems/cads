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
bool store_profile(sqlite3_stmt*, profile);
profile fetch_profile(sqlite3_stmt*, uint64_t);
void close_db(sqlite3 *db = nullptr, sqlite3_stmt* stmt = nullptr,sqlite3_stmt* stmt2 = nullptr); 
void store_profile_thread(moodycamel::BlockingReaderWriterQueue<cads::profile> &db_fifo);


}

#endif