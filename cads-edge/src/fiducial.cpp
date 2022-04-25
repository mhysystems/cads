#include <fiducial.h>
#include <opencv2/imgproc.hpp>
#include <opencv2/imgcodecs.hpp>

#include <cmath>
#include <chrono>
#include <fmt/chrono.h>


using namespace std;
using namespace cv;

namespace cads {

Mat make_fiducial(double x_res, double y_res, double fy_mm, double fx_mm, double fg_mm) {

	int nrows = round(fy_mm / y_res); // Y direction(belt length)
	int gap = round(fg_mm / y_res);
	int ncols = round(fx_mm / x_res); // X direction
  int border_y = ceil(0 / y_res);
  int border_x = ceil(0 / x_res);

	Mat fiducial(gap + 2*nrows + 2*border_y,ncols + 2*border_x,CV_32F,{1.0});

	rectangle(fiducial,{border_x,border_y,ncols,nrows},{0.0},-1);
	rectangle(fiducial,{border_x,border_y+nrows+gap,ncols,nrows},{0.0},-1);

	return fiducial;

}

bool fiducial_as_image(Mat m, std::string suf) {
  auto now = fmt::format("{:%Y%m%d%H%M%S}",chrono::system_clock::now());
  auto mc = m.clone();
  patchNaNs(mc);

  double minVal, maxVal;
  minMaxLoc( mc, &minVal, &maxVal);
  auto f = 255.0 / (maxVal-minVal);
  
  for(int i = 0; i < mc.rows; i++) {
    float* mci = mc.ptr<float>(i);
    for(int j = 0; j < mc.cols; j++) {
        mci[j] = (mci[j] - minVal) * f;
    }
  }

  return imwrite("fiducial"+now+suf+".png", mc);
}

bool mat_as_image(Mat m, double z_threshold) {
	cv::Mat grey;
	cv::threshold(m,grey,z_threshold,1.0,cv::THRESH_BINARY);

	return fiducial_as_image(grey,"b");

}

double search_for_fiducial(cv::Mat belt, cv::Mat fiducial, double c_threshold, double z_threshold) {
	
	cv::Mat black_belt;
	cv::threshold(belt.colRange(0,fiducial.cols*2),black_belt,z_threshold,1.0,cv::THRESH_BINARY);

	cv::Mat out(black_belt.rows - fiducial.rows + 1,black_belt.cols - fiducial.cols + 1, CV_32F);
	cv::matchTemplate(black_belt,fiducial,out,cv::TM_SQDIFF_NORMED);

	double minVal;
  minMaxLoc( out, &minVal);

	return minVal;

}

}