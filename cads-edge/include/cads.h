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
//using profile = std::tuple<uint64_t,double,std::vector<int16_t>>;
using profile = struct profile{uint64_t y; double x_off;std::vector<int16_t> z;};
using profile_window = std::deque<profile>;
constexpr int16_t InvalidRange16Bit = 0x8000;

std::string slurpfile(const std::string_view path, bool binaryMode = true);
void process_flatbuffers();

}

#endif