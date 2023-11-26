#include <ranges>
#include <cmath>

#include <vec_nan_free.h>

namespace cads
{
  void swap(vector_NaN_free& first, vector_NaN_free& second)
  {
    std::swap(first.data, second.data);
  }

  vector_NaN_free vector_NaN_free::operator+(vector_NaN_free&& rhs)
  {
    data.insert(data.end(), rhs.data.begin(), rhs.data.end());
    return std::move(*this);
  }

  vector_NaN_free::vector_NaN_free(vector_NaN_free&& rhs) : data(std::move(rhs.data))
  {

  }
  
  vector_NaN_free::vector_NaN_free(const z_type& z)
  {
    namespace sr = std::ranges;

    auto r = z | sr::views::filter([](float a){ return !std::isnan(a); }); 
    data = {r.begin(),r.end()};
  };

}