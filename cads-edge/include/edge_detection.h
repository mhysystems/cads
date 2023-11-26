#pragma once

#include <profile_t.h>

namespace cads 
{

  struct Dbscan {
    double InClusterRadius;
    size_t MinPoints;
    double MergeRadius;
    size_t MaxClusters;
  };


  /** @brief Searches for belt edge using dbScan clustering.

  aaa

  @note Profile needs to be filtered of spikes and is not reliable with noisy data as NaNs appear though the belt.

  @param z vector of samples representing a profile
  @param config DbScan configuration
  @return set of pointer pairs pointing into z.

  */
  z_cluster dbscan(cads::zrange z, const cads::Dbscan config);

} // namespace cads