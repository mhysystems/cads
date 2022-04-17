#include <boost/math/statistics/linear_regression.hpp>
#include <vector>
#include <numeric>
#include <ranges>

namespace cads {
  using boost::math::statistics::simple_ordinary_least_squares;

  std::tuple<double,double> linear_regression(std::vector<int16_t> yy,int left_edge_index, int right_edge_index) {
    namespace sr = std::ranges;

    auto r = yy | sr::views::take(right_edge_index) | sr::views::drop(left_edge_index) | sr::views::filter([](int16_t a){ return a != 0x8000; });
    
    std::vector<double> y(r.begin(),r.end());
    
    std::vector<double> x(y.size());

    std::iota(x.begin(), x.end(), 0);
    return simple_ordinary_least_squares(x, y);
  }

}
