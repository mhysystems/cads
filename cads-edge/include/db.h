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
//sqlite3_stmt* fetch_profile_statement(sqlite3*);
//bool store_profile(sqlite3_stmt*, const profile&);
coro<std::tuple<profile, int>> fetch_belt_coro();

//void close_db(sqlite3 *db = nullptr, sqlite3_stmt* stmt = nullptr,sqlite3_stmt* stmt2 = nullptr); 
coro<int, profile> store_profile_coro(profile p);
int store_profile_parameters(double y_res, double x_res, double z_res, double z_off, double encoder_res);
std::tuple<double,double,double,double,double,int> fetch_profile_parameters(std::string name);


}

