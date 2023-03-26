#pragma once

#include <string>
#include <tuple>

#include <opencv2/core/mat.hpp>


namespace cads {  
  cv::Mat make_fiducial(double x_res, double y_res);
  cv::Mat make_fiducial(double x_res, double y_res, double fy_mm, double fx_mm, double fg_mm);
  bool mat_as_image(cv::Mat m, double z_threshold);
  double search_for_fiducial(cv::Mat belt, cv::Mat fiducial, double z_threshold);
  std::tuple<double,cv::Point> search_for_fiducial(cv::Mat belt, cv::Mat fiducial, cv::Mat& black_belt,cv::Mat& out, double z_threshold);
  bool mat_as_image(cv::Mat m, std::string suf = "f");

}