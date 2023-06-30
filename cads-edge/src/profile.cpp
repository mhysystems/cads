#include <limits>
#include <cmath>
#include <vector>
#include <ranges>
#include <tuple>
#include <algorithm>
#include <numeric>
#include <bit>
#include <cassert>

#include <profile.h>
#include <constants.h>
#include <regression.h>
#include <edge_detection.h>
#include <stats.hpp>
#include <interpolation.h>
#include <filters.h>

namespace cads
{

  std::string ClusterErrorToString(ClusterError error)
  {
    using namespace std::string_literals;
    switch (error)
    {
    case ClusterError::NoClusters:
      return "NoClusters"s;
    case ClusterError::ExcessiveClusters:
      return "ExcessiveClusters"s;
    case ClusterError::ExcessiveNeigbours:
      return "ExcessiveNeigbours"s;
    default:
      return "None"s;
    }
  }
  
  std::tuple<z_element, z_element> find_minmax_z(const z_type &ps)
  {

    auto z_min = std::numeric_limits<z_element>::max();
    auto z_max = std::numeric_limits<z_element>::lowest();

    for (auto z : ps)
    {
      if (!std::isnan(z))
      {
        z_min = std::min(z, z_min);
        z_max = std::max(z, z_max);
      }
    }

    return {z_min, z_max};
  }

  std::vector<std::tuple<double, z_element>> histogram(const z_type &ps, z_element min, z_element max, double size)
  {

    const float dz = ((float)size - 1) / (max - min);
    std::vector<std::tuple<double, z_element>> hist((size_t)size, {0, 0.0f});

    for (auto z : ps)
    {
      if (!std::isnan(z))
      {
        int i = int((z - min) * dz);
        hist[i] = {((float)i + 2) * (1 / dz) + min, 1 + get<1>(hist[i])};
      }
    }

    std::ranges::sort(hist, [](auto a, auto b)
                      { return get<1>(a) > get<1>(b); });
    return hist;
  }


  auto mk_zrange(z_type &z) {
    return zrange{z.begin(),z.end()};
  }

  auto mk_zrange(z_cluster &c) {
    return zrange{c.front().begin(),c.back().end()};
  }

  auto average_zrange(zrange r) {
    
    double size = 0;
    double sum = 0;
    
    for(auto i = begin(r); i < end(r); ++i)
    {
      if(!std::isnan(*i)){
        sum += *i;
        size++;
      };
    }

    return sum / size;
  }

  auto sumr(zrange r) {
    
    double size = 0;
    double sum = 0;
    
    for(auto i = begin(r); i < end(r); ++i)
    {
      if(!std::isnan(*i)){
        sum += *i;
        size++;
      };
    }

    return std::make_tuple(sum,size);
  }

 
  auto average_cluster(const z_cluster& a) {
    
    double size = 0;
    double sum = 0;
    for(auto r : a) {
      auto [s,l] = sumr(r);
      sum += s;
      size += l;
    }

    return sum / size;

  }

  auto construct_ztype(const z_cluster& zv) {
    
    if(zv.size() < 1) {
      return z_type();
    }
    
    z_type out;

    for(auto r : zv) {
      std::copy(begin(r),end(r),std::back_inserter(out));
    }

    return out;
  }

  auto construct_ztype(zrange zv) {
    
    if(zv.size() < 1) {
      return z_type();
    }
    
    z_type out;

    std::copy(begin(zv),end(zv),std::back_inserter(out));

    return out;
  }

  // Return sequence of clustered z values and offset into profile
  // Input is a slice of profile, hence tracking offset
  std::tuple<zrange,size_t> cluster(zrange z, z_element origin, z_element max_diff = dbscan_config.InCluster, z_element dis = dbscan_config.MinPoints )
  {

    namespace sr = std::ranges;

    if (empty(z))
    {
      return {z,0};
    }

    auto i = begin(z);
    z_element e = *i;
    size_t d = 0;
    for(; i < end(z); ++i)
    {
      if(std::isnan(*i)) continue;

      if(std::abs(*i - origin) < max_diff) {
        e = *i;
        ++d;
      } 
      else {
        break;
      }
    }

    if (d > dis)
    {
      auto [c,cd] = cluster({i,end(z)},e);
      if(cd > 0) {
        return {{begin(z),end(c)},d + cd};
      }else{
        return {{begin(z),i},d};
      }
    }else {
      return {{i,i},0};
    }
  }
  
  
  std::tuple<zrange,size_t> cluster(z_type &z)
  {

    return cluster(mk_zrange(z),z[0]);
  }

  void cluster_merge(z_clusters& group, zrange c, double in_cluster = dbscan_config.InCluster) 
  {

    if(group.size() < 1) {
      group.push_back({c});
      return;
    }
    
    auto c_avg =  average_zrange(c);

    auto& x = group.back().back();
    auto group_avg = average_zrange(x);
    
    if(std::abs(c_avg - group_avg) < in_cluster ) {
        x = {begin(x),end(c)};
      
    }else {
      group.push_back({c});      
    }

  }

  z_clusters dbscan(zrange z, std::vector<z_cluster> &&group = {}, size_t min_points = dbscan_config.MinPoints)
  {

    auto [a,d] = cluster(z,*begin(z));

    if(d > min_points) {
      cluster_merge(group,a);
    }

    if (end(a) != end(z))
    {
      group = dbscan({end(a), end(z)}, std::move(group));
    }

    return group;
  }

  z_clusters dbscan(z_type &z)
  {
    return dbscan(mk_zrange(z));

  }

  void recontruct_z(z_type & z,const z_clusters& group) {

    auto i = z.begin();
    for(const auto &g : group) {
      
      for(auto r : g) {

        for(; i < begin(r); ++i) {
          *i = std::numeric_limits<z_element>::quiet_NaN();
        }
        
        auto re = std::bit_cast<z_type::iterator>(end(r));
        nan_interpolation_last(i,re);
        i = re;

      }
    }

    for(;i < z.end();++i) {
      *i = std::numeric_limits<z_element>::quiet_NaN();
    }

  }


  double average(z_type & v){
    if(v.empty()){
        return 0;
    }

    auto const count = static_cast<float>(v.size());
    return std::reduce(v.begin(), v.end()) / count;
  }

  std::tuple<double,double,size_t,size_t,ClusterError> pulley_levels_clustered(z_type &z, std::function<double(const z_type &)> estimator)
  {
 
    spike_filter(z);
    auto clusters = dbscan(z);
    auto avg_l = 0.0;
    auto avg_r = 0.0;
    size_t left_edge = 0;
    size_t right_edge = z.size();
    ClusterError error = ClusterError::None;

    if(clusters.size() < 2) {
      error = ClusterError::NoClusters;
    }else if(clusters.size() == 2) {
      // Only one side of pulley detected.
      // Check heights. Assumes belt is higher than pulley.
      // Pulley found on left of belt if index is 1 else right
      auto pulley = (*begin(clusters[0][0]) < *begin(clusters[1][0])) ? 0 : 1;
      auto belt = -1*pulley + 1;

      auto b = mk_zrange(clusters[pulley]);
      auto avg = estimator(construct_ztype(b));
      avg_l = avg;
      avg_r = avg;
 
      left_edge = std::distance(z.begin(),clusters[belt].front().begin());
      right_edge = std::distance(z.begin(),clusters[belt].back().end());

      if(clusters[pulley].size() > 1) {
        error = ClusterError::ExcessiveNeigbours;
      }

    }else if(clusters.size() == 3){

      auto size = clusters[1].size();
      left_edge = std::distance(z.begin(),clusters[1].front().begin());
      right_edge = std::distance(z.begin(),clusters[1].back().end());
      
      avg_l = estimator(construct_ztype(mk_zrange(clusters.front())));
      avg_r = estimator(construct_ztype(mk_zrange(clusters.back())));

      if(size > 1) {
        error = ClusterError::ExcessiveNeigbours;
      }

    }else {
      error = ClusterError::ExcessiveClusters;

      auto cluster_size = clusters.size();

      left_edge = std::distance(z.begin(),clusters[1].front().end());
      right_edge = std::distance(z.begin(),clusters[cluster_size - 2].back().end());
      avg_l = estimator(construct_ztype(mk_zrange(clusters.front())));
      avg_r = estimator(construct_ztype(mk_zrange(clusters.back())));
    }
    
    return {avg_l,avg_r,left_edge,right_edge,error};
  }


  bool operator==(const profile &a, const profile &b)
  {
    auto tx = a.x_off == b.x_off;
    auto ty = a.y == b.y;
    auto tz = std::ranges::equal(a.z, b.z);
    return tx && ty && tz;
  }

  z_type trim_nan(const z_type &z)
  {

    auto left = std::find_if(z.begin(), z.end(), [](z_element z)
                             { return !std::isnan(z); });
    auto right = std::find_if(z.rbegin(), z.rend(), [](z_element z)
                              { return !std::isnan(z); });

    return {left, right.base()};
  }

  void constraint_substitute(z_type &z, z_element z_min, z_element z_max, z_element sub)
  {
    for (auto &e : z)
    {
      if (!std::isnan(e))
      {
        if (e < z_min || e > z_max)
        {
          e = sub;
        }
      }
    }
  }

} // namespace cads