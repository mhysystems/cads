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

  
  std::tuple<vector_NaN_free,vector_NaN_free> vector_NaN_free::yx(z_type::const_iterator begin, z_type::const_iterator end)
  {

    if( std::distance(begin,end) < 1) {
      return {z_type(),z_type()};
    }
    
    vector_NaN_free y,x;

    for(auto i = begin; i < end;++i) {
      if(!std::isnan(*i)) { 
        y.data.push_back(*i);
        x.data.push_back(z_type::value_type(std::distance(begin,i)));
      }
    } 

    return {std::move(y),std::move(x)};
  }

}