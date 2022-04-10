#ifndef CADS_H
#define CADS_H 1

#include <tuple>
#include <vector>
#include <deque>
#include <cstdint>
#include <string>
#include <istream>

#include <opencv2/core/mat.hpp>

namespace cads {
using gocator_profile = std::tuple<uint64_t,double,std::vector<int16_t>>;
using profile = struct profile{uint64_t y; double x_off; double left_edge; double right_edge; std::vector<int16_t> z;};
using profile2 = struct profile2{uint64_t y; double x_off; std::vector<float> z;};
using profile_window = std::deque<profile>;
using profile_window2 = std::deque<profile2>;

std::string slurpfile(const std::string_view path, bool binaryMode = true);
void process_flatbuffers();
void process_flatbuffers2();

}

#endif