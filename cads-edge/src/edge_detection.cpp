#include <cmath>
#include <tuple>

#include <edge_detection.h>
#include <vec_nan_free.h>
#include <regression.h>

namespace 
{
  // Return sequence of clustered z values and offset into profile
  // Input is a slice of profile, hence tracking offset
  std::tuple<cads::zrange,size_t> cluster1D(cads::zrange z, cads::z_element max_diff)
  {

    using namespace std;

    if (empty(z))
    {
      return {z,0};
    }

    auto i = begin(z);

    for(;std::isnan(*i) && i < end(z); ++i);

    auto s = i;
    size_t cnt = 0;

    for(; i < end(z); ++i)
    {
      if(std::isnan(*i)) continue;
      
      cads::z_element sum = 0;
      // sum 2nd order difference, skipping NaNs
      auto j = i+1;
      for(auto d = 0; j < end(z) && d < 2 && sum <= max_diff; ++j) {
        if(std::isnan(*j)) continue;
        sum += std::abs(*j - *i);
        ++d;
      }
      
      if( sum > max_diff )
      { 
        // Edge of belt will be an NaN
        for(++i; !std::isnan(*i) && i < end(z); ++i);
        break;
      }

      cnt++;
    }

    return {{s,i},cnt};
  }

  auto cluster_regression(cads::zrange last_cluster, cads::zrange append_cluster)
  {
    auto cluster_params = cads::linear_regression(cads::vector_NaN_free::yx(last_cluster.cbegin(),last_cluster.end()));
    auto append_params = cads::linear_regression(cads::vector_NaN_free::yx(append_cluster.cbegin(),append_cluster.end()));
    return std::tuple{cluster_params,append_params};
  }

  void cluster_merge1D(cads::z_cluster& group, cads::zrange c, const double in_cluster) 
  {

    if(group.size() < 1) {
      group.push_back({c});
      return;
    }
    
    auto& x = group.back();
    
    auto [x_params, c_params] = cluster_regression(x,c);
    if(std::abs(x_params.intercept - c_params.intercept) < in_cluster ) {
    //if(std::abs(*c.begin() - *(x.end()-1)) < in_cluster ) {
        x = {begin(x),end(c)};
      
    }else {
      group.push_back({c});      
    }

  }

  cads::z_cluster dbscan1D(cads::zrange z, cads::z_cluster &&group, const cads::Dbscan config)
  {
    using namespace std;
    auto a_cluster = z;
    size_t cnt = 0;
    do {
      std::tie(a_cluster,cnt) = cluster1D(z,config.InClusterRadius);

      if( cnt > config.MinPoints) {
        cluster_merge1D(group,a_cluster,config.ZMergeRadius);
      }

      z = {end(a_cluster), end(z)};

    }while(end(a_cluster) < end(z) && group.size() <= config.MaxClusters);

    return group;
  }

}


namespace cads 
{
  z_cluster dbscan(cads::zrange z, const cads::Dbscan config)
  {
    return ::dbscan1D({std::begin(z),std::end(z)},{},config);
  }

} // namespace cads