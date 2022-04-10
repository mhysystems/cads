#pragma once

#include <vector>
#include <deque>
#include <cstdint>

#include <opencv2/core/mat.hpp>

namespace cads2 {
using profile = struct profile{uint64_t y; double x_off; std::vector<float> z;};
using window = std::deque<profile>;

cv::Mat window_to_mat(std::deque<profile> win, double x_res);

}
