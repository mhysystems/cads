#pragma once

#include <tuple>
#include <vector>

namespace cads {
std::tuple<double,double> linear_regression(std::vector<int16_t> y, int left_edge_index, int right_edge_index);
}