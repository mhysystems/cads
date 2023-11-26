#pragma once

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

    private:
    vector_NaN_free() = delete;
	vector_NaN_free(const vector_NaN_free&) = delete;
	vector_NaN_free& operator=(const vector_NaN_free&) = delete;
     vector_NaN_free& operator=(vector_NaN_free&&) = delete;

  };

}