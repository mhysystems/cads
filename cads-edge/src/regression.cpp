#include <vector>
#include <numeric>
#include <ranges>
#include <cmath>
#include <limits>

#include <Eigen/Dense>

#include <regression.h>
#include <constants.h>

namespace cads
{

  std::tuple<double, double> linear_regression(const z_type &yy, int left_edge_index, int right_edge_index) {
    
    namespace sr = std::ranges;
    using namespace Eigen;

    auto r = yy | sr::views::take(right_edge_index) | sr::views::drop(left_edge_index) | sr::views::filter([](int16_t a)
                                                                                                           { return !NaN<z_element>::isnan(a); });

    std::vector<double> yr(r.begin(), r.end());
    auto y = Map<VectorXd>(yr.data(), yr.size());

    MatrixXd A(y.size(),2);
    A.setOnes();
    A.col(1) = ArrayXd::LinSpaced(y.size(), 0, y.size()-1);
    MatrixXd s = A.bdcSvd(Eigen::ComputeThinU | Eigen::ComputeThinV).solve(y); 
    
    return {s(0,0),s(1,0)};
  }


  void regression_compensate(z_type &z, int left_edge_index, int right_edge_index, double gradient)
  {
    namespace sr = std::ranges;

    auto f = z | sr::views::take(right_edge_index) | sr::views::drop(left_edge_index);
    std::transform(f.begin(), f.end(), f.begin(), [gradient, i = 0](z_element v) mutable -> z_element
                   { return v != NaN<z_element>::value ? v - (gradient * i++) : NaN<z_element>::value; });
  }

  int correlation_lowest(float *a, size_t al, float *b, size_t bl)
  {

    using namespace Eigen;

    auto va = Map<VectorXf>(a, al);
    auto vb = Map<VectorXf>(b, bl);

    auto bs = bl - 1;
    auto ds = al - bl;
    auto c = 0;
    auto r = std::numeric_limits<float>::max();

    for (int i = 0; i <= ds; ++i)
    {
      auto cr = std::abs((va(seq(i, i + bs)) - vb).sum());
      if (cr < r)
      {
        r = cr;
        c = i;
      }
    }

    return c;
  }

}
