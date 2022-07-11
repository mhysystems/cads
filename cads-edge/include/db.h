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

bool create_db(std::string name = "");
coro<int, std::tuple<int,int,profile>,1> store_profile_coro(std::string name = "");
coro<std::tuple<int,profile>> fetch_belt_coro(int revid, int last_idx, int size = 256, std::string name = "");

 int store_profile_parameters(double y_res, double x_res, double z_res, double z_off, double encoder_res, double z_max, double z_min = 0, std::string name = "");
std::tuple<double,double,double,double,double,double,double,int> fetch_profile_parameters(std::string name);


}

