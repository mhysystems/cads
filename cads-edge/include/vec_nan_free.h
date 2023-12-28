#pragma once

#include <tuple>

#include <profile_t.h>

namespace cads
{
  struct vector_NaN_free
  {

    vector_NaN_free(const z_type& z);
    vector_NaN_free operator+(vector_NaN_free&&);
    friend void swap(vector_NaN_free& , vector_NaN_free& );
    vector_NaN_free(vector_NaN_free&&);
    z_type data;

    static std::tuple<vector_NaN_free,vector_NaN_free> yx(z_type::const_iterator, z_type::const_iterator);

    private:
    vector_NaN_free() = default;
	  vector_NaN_free(const vector_NaN_free&) = delete;
	  vector_NaN_free& operator=(const vector_NaN_free&) = delete;
    vector_NaN_free& operator=(vector_NaN_free&&) = delete;

  };

}