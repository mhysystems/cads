#include <boost/math/statistics/linear_regression.hpp>
#include <vector>
#include <numeric>
#include <ranges>
#include <Eigen/Core>
#include <cmath>
#include <limits>

#include <regression.h>
#include <constants.h>

namespace cads
{
  using boost::math::statistics::simple_ordinary_least_squares;

  std::tuple<double, double> linear_regression(const z_type &yy, int left_edge_index, int right_edge_index)
  {
    namespace sr = std::ranges;

    auto r = yy | sr::views::take(right_edge_index) | sr::views::drop(left_edge_index) | sr::views::filter([](int16_t a)
                                                                                                           { return !NaN<z_element>::isnan(a); });

    std::vector<double> y(r.begin(), r.end());

    std::vector<double> x(y.size());

    std::iota(x.begin(), x.end(), 0);
    return simple_ordinary_least_squares(x, y);
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
