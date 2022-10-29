//  (C) Copyright Nick Thompson 2018.
//  (C) Copyright Matt Borland 2020.
//  Use, modification and distribution are subject to the
//  Boost Software License, Version 1.0. (See accompanying file
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <algorithm>
#include <iterator>
#include <ranges>
#include <tuple>

namespace cads
{
  template <class ForwardIterator>
  auto interquartile_range(ForwardIterator first, ForwardIterator last)
  {
    using Real = typename std::iterator_traits<ForwardIterator>::value_type;
    static_assert(!std::is_integral_v<Real>, "Integer values have not yet been implemented.");
    auto m = std::distance(first, last);

    auto k = m / 4;
    auto j = m - (4 * k);
    // m = 4k+j.
    // If j = 0 or j = 1, then there are an even number of samples below the median, and an even number above the median.
    //    Then we must average adjacent elements to get the quartiles.
    // If j = 2 or j = 3, there are an odd number of samples above and below the median, these elements may be directly extracted to get the quartiles.

    if (j == 2 || j == 3)
    {
      auto q1 = first + k;
      auto q3 = first + 3 * k + j - 1;
      std::nth_element(first, q1, last);
      Real Q1 = *q1;
      std::nth_element(q1, q3, last);
      Real Q3 = *q3;
      return std::make_tuple(Q1,Q3);
    }
    else
    {
      // j == 0 or j==1:
      auto q1 = first + k - 1;
      auto q3 = first + 3 * k - 1 + j;
      std::nth_element(first, q1, last);
      Real a = *q1;
      std::nth_element(q1, q1 + 1, last);
      Real b = *(q1 + 1);
      Real Q1 = (a + b) / 2;
      std::nth_element(q1, q3, last);
      a = *q3;
      std::nth_element(q3, q3 + 1, last);
      b = *(q3 + 1);
      Real Q3 = (a + b) / 2;
      return std::make_tuple(Q1,Q3);
    }
  }

  template <class Container>
  auto interquartile_range(Container &z)
  {
    using namespace std::ranges;

    auto f = z | views::filter([](z_element a)
                               { return !std::isnan(a); });
    
    Container tmp(f.begin(),f.end()); // range filter view iterator nor random access
    return interquartile_range(std::begin(tmp), std::end(tmp));
  }

}