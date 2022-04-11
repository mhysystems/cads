#pragma once

#include <vector>
#include <deque>
#include <cstdint>

#include <opencv2/core/mat.hpp>

#include <cads.h>

namespace cads {
using window = std::deque<profile>;

cv::Mat window_to_mat(window win, double x_res);

}
