#pragma once

#include <opencv2/core/mat.hpp>

namespace cads {
  cv::Mat make_fiducial(double x_res, double y_res, double fy_mm, double fx_mm, double fg_mm);
  bool fiducial_as_image(cv::Mat m);
}