#pragma once

#include <opencv2/core/mat.hpp>
#include <string>

namespace cads {
  cv::Mat make_fiducial(double x_res, double y_res, double fy_mm, double fx_mm, double fg_mm);
  bool mat_as_image(cv::Mat m, double z_threshold);
  double search_for_fiducial(cv::Mat belt, cv::Mat fiducial, double c_threshold, double z_threshold);
  bool fiducial_as_image(cv::Mat m, std::string suf = "f");
}