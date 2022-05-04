#ifndef CADS_H
#define CADS_H 1

#include <tuple>
#include <vector>
#include <deque>
#include <cstdint>
#include <string>

#include <profile.h>

namespace cads {
using gocator_profile = std::tuple<uint64_t,double,std::vector<int16_t>>;
using profile_window = std::deque<profile>;

void process_daily();
void store_profile_only();
void process_one_revolution();
void process_experiment();

}

#endif