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
  int border_y = ceil(5 / y_res);
  int border_x = ceil(5 / x_res);

	Mat fiducial(gap + 2*nrows + 2*border_y,ncols + 2*border_x,CV_32F,{1.0});

	rectangle(fiducial,{border_x,border_y,ncols,nrows},{0.0},-1);
	rectangle(fiducial,{border_x,border_y+nrows+gap,ncols,nrows},{0.0},-1);

	return fiducial;

}

bool fiducial_as_image(Mat m) {
  auto now = fmt::format("{:%Y%m%d%H%M%S}",chrono::system_clock::now());
  auto mc = m.clone();

  for(int i = 0; i < mc.rows; i++) {
    float* mci = mc.ptr<float>(i);
    for(int j = 0; j < mc.cols; j++) {
        mci[j] *= 255.0;
    }
  }

  return imwrite("fiducial"+now+".png", mc);
}

}