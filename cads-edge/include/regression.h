#pragma once

#include <tuple>
#include <vector>
#include <profile.h>

namespace cads {
std::tuple<double,double> linear_regression(const z_type& y, int left_edge_index, int right_edge_index);
void regression_compensate(z_type& p,int left_edge_index, int right_edge_index, double gradient);
}