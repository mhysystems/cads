#pragma once

#include <profile.h>
#include <tuple>
#include <constants.h>

namespace cads 
{
  /** @brief Searches for belt edge using NaN.

  Uses the fact the greatest consecutive sequence of NaNs are generally found at the edge of the belt.
  Starts from the middile of the profile and searches outwards.

  @note Profile needs to be filtered of spikes and is not reliable with noisy data as NaNs appear though the belt.

  @param z vector of samples representing a profile
  @param len Number of consecutive NaN
  @return Left and Right index of an edge in z

  */
  std::tuple<int,int> find_profile_edges_nans(const z_type& z, int len);

  /** @brief Searches for belt edge using NaN.

  Like find_profile_edges_nans() but starts from the boundaries.

  @note Profile needs to be filtered of spikes and is not reliable with noisy data as NaNs appear though the belt.

  @param z vector of samples representing a profile
  @param len Number of consecutive NaN
  @return Left and Right index of an edge in z

  */
  std::tuple<int,int> find_profile_edges_nans_outer(const z_type& z, int len );

  std::tuple<int,int> find_profile_edges_sobel(const z_type& z, int len);

  std::tuple<int,int> find_profile_edges_zero(const z_type& z);

} // namespace cads