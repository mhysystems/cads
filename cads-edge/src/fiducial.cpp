#include <cmath>
#include <chrono>

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wuseless-cast"
#pragma GCC diagnostic ignored "-Wshadow"
#pragma GCC diagnostic ignored "-Wunused-parameter"
#pragma GCC diagnostic ignored "-Wconversion"
#pragma GCC diagnostic ignored "-Wnon-virtual-dtor"
#pragma GCC diagnostic ignored "-Wreorder"
#pragma GCC diagnostic ignored "-Wuseless-cast"
#pragma GCC diagnostic ignored "-Wdangling-reference"

#include <fmt/chrono.h>
#include <opencv2/imgproc.hpp>
#include <opencv2/imgcodecs.hpp>

#pragma GCC diagnostic pop

#include <constants.h>
#include <fiducial.h>


using namespace std;
using namespace cv;


namespace cads {

Mat make_fiducial(double x_res, double y_res, double fy_mm, double fx_mm, double fg_mm) {

	int nrows = round(fy_mm / y_res); // Y direction(belt length)
	int gap = round(fg_mm / y_res);
	int ncols = round(fx_mm / x_res); // X direction
  int border_y = ceil(fy_mm / y_res);
  int border_x = ceil(10 / x_res);

	Mat fiducial(gap + 2*nrows + 2*border_y,ncols + border_x,CV_32F,{1.0});

	rectangle(fiducial,{0,border_y,ncols,nrows},{0.0},-1);
	rectangle(fiducial,{0,border_y+nrows+gap,ncols,nrows},{0.0},-1);

	return fiducial;

}

Mat make_fiducial(double x_res, double y_res, Fiducial config) {

	auto fnrows = config.fiducial_y;
	auto fgap = config.fiducial_gap;
	auto fncols = config.fiducial_x;
  
	return make_fiducial(x_res,y_res,fnrows,fncols,fgap);

}

bool mat_as_image(Mat m, std::string pre) {
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

  return imwrite(pre+"_"+now+".png", mc);
}

bool mat_as_image(Mat m, double z_threshold) {
	cv::Mat grey;
	cv::threshold(m,grey,z_threshold,1.0,cv::THRESH_BINARY);

	return mat_as_image(grey,"threshhold");

}

double search_for_fiducial(cv::Mat belt, cv::Mat fiducial, double z_threshold) {
	
	cv::Mat black_belt;
	cv::threshold(belt.colRange(0,fiducial.cols*1.5),black_belt,z_threshold,1.0,cv::THRESH_BINARY);

	cv::Mat out(black_belt.rows - fiducial.rows + 1,black_belt.cols - fiducial.cols + 1, CV_32F,cv::Scalar::all(0.0f));
	cv::matchTemplate(black_belt,fiducial,out,cv::TM_SQDIFF_NORMED);

	double minVal;
  minMaxLoc( out, &minVal);

	return minVal;

}

std::tuple<double,cv::Point> search_for_fiducial2(cv::Mat belt, cv::Mat fiducial, cv::Mat& black_belt,cv::Mat& out, double z_threshold) {
	
	cv::threshold(belt,black_belt,z_threshold,1.0,cv::THRESH_BINARY);
	cv::matchTemplate(black_belt,fiducial,out,cv::TM_SQDIFF_NORMED);

	double minVal;
  cv::Point minLoc;
  minMaxLoc( out, &minVal, nullptr, &minLoc);

	return {minVal,minLoc};

}

std::tuple<double,cv::Point> search_for_fiducial(cv::Mat belt, cv::Mat fiducial, cv::Mat& black_belt,cv::Mat& out, double z_threshold) {
	
	cv::threshold(belt.rowRange(0,fiducial.rows*2),black_belt,z_threshold,1.0,cv::THRESH_BINARY);
	cv::matchTemplate(black_belt,fiducial,out,cv::TM_SQDIFF_NORMED);

	double minVal;
  cv::Point minLoc;
  minMaxLoc( out, &minVal, nullptr, &minLoc);

	return {minVal,minLoc};

}

#if 0
double samples_contains_fiducial_gpu(CadsMat belt, CadsMat fiducial, double c_threshold, double z_threshold) {
	
	CadsMat black_belt; //.colRange(0,fiducial.cols*2)
	cv::threshold(belt.colRange(0,fiducial.cols*2),black_belt,z_threshold,1.0,cv::THRESH_BINARY);

	CadsMat out(black_belt.rows - fiducial.rows + 1,black_belt.cols - fiducial.cols + 1, CV_32F);
	cv::matchTemplate(black_belt,fiducial,out,cv::TM_SQDIFF_NORMED);

	double minVal; double maxVal; cv::Point minLoc; cv::Point maxLoc;
  minMaxLoc( out, &minVal, &maxVal, &minLoc, &maxLoc, cv::Mat() );
	
	return minVal;

}
#endif

}
