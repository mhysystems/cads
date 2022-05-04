#include <boost/math/statistics/linear_regression.hpp>
#include <vector>
#include <numeric>
#include <ranges>

#include <regression.h>
#include <constants.h>

namespace cads {
  using boost::math::statistics::simple_ordinary_least_squares;

  std::tuple<double,double> linear_regression(const z_type& yy,int left_edge_index, int right_edge_index) {
    namespace sr = std::ranges;

    auto r = yy | sr::views::take(right_edge_index) | sr::views::drop(left_edge_index) | sr::views::filter([](int16_t a){ return a != NaN<z_element>::value; });
    
    std::vector<double> y(r.begin(),r.end());
    
    std::vector<double> x(y.size());

    std::iota(x.begin(), x.end(), 0);
    return simple_ordinary_least_squares(x, y);
  }
  
  void regression_compensate(z_type& z,int left_edge_index, int right_edge_index, double gradient) {
    namespace sr = std::ranges;

    auto f = z | sr::views::take(right_edge_index) | sr::views::drop(left_edge_index);
    std::transform(f.begin(), f.end(), f.begin(),[gradient, i = 0](z_element v) mutable -> z_element{return v != NaN<z_element>::value ? v - (gradient*i++) : NaN<z_element>::value;});
  }

}
