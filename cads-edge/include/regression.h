#pragma once

#include <tuple>
#include <vector>
#include <profile_t.h>
#include <vec_nan_free.h>

namespace cads {

struct linear_params
{
  double gradient;
  double intercept;
};

linear_params linear_regression(std::tuple<vector_NaN_free,vector_NaN_free>);

void regression_compensate(z_type& z, size_t left_edge_index, size_t right_edge_index, double gradient, double intercept = 0);
void regression_compensate(z_type& z, double gradient, double intercept = 0);

std::function<std::tuple<double,double,double>(z_type)>  mk_curvefitter(float init_height, float init_x_offset, float init_z_offset, const int width_n, const double x_res, const double z_res);
std::function<double(std::vector<float>)>  mk_pulleyfitter(float z_res, float init_z = 0);

}