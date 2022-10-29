#pragma once

#include <tuple>
#include <vector>
#include <profile.h>

namespace cads {
std::tuple<double,double> linear_regression(const z_type& y, int left_edge_index, int right_edge_index);
void regression_compensate(z_type& p,int left_edge_index, int right_edge_index, double gradient);
int correlation_lowest(float * a, size_t al, float* b, size_t bl) ;
std::function<std::tuple<double,double,double>(z_type)>  mk_curvefitter(float init_height, float init_x_offset, float init_z_offset, const int width_n, const double x_res, const double z_res);

template<typename T, typename T2> auto correlation_lowest(T aa, T2  ab) {
	if(aa.size() < 1 || ab.size() < 1) return 0;
	
  float *a,*b;
  size_t as,bs;

  if(aa.size() >= ab.size()) {
    a = aa.data();
    b = ab.data();
    as = aa.size();
    bs = ab.size();
  }else {
    a = ab.data();
    b = aa.data();
    as = ab.size();
    bs = aa.size();
  }

  auto r = correlation_lowest(a,as,b,bs);
	return aa.size() >= ab.size() ? r : -r;

}


}