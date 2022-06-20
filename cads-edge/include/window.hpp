#pragma once

#include <vector>
#include <deque>
#include <cstdint>
#include <tuple>

#include <opencv2/core/mat.hpp>

#include <profile.h>

namespace cads {
using window = std::deque<profile>;

cv::Mat window_to_mat(const window& win, double x_res);
cv::Mat window_to_mat_fixed(const window& win, int width);
std::vector<std::tuple<double,z_element>> histogram(const window& ps, z_element min, z_element max, double size = 100);
std::tuple<z_element,z_element> find_minmax_z(const window& ps);
std::tuple<double,double> x_minmax(window win, double x_res); 
double left_edge_avg_height(const cv::Mat& belt, const cv::Mat& fiducial);
std::tuple<z_element,z_element> barrel_offset(const window& win, double z_resolution, double z_height_mm);

}
