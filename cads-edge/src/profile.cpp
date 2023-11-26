#include <limits>
#include <cmath>
#include <vector>
#include <ranges>
#include <tuple>
#include <algorithm>
#include <numeric>
#include <bit>
#include <cassert>
#include <iterator>

#include <profile.h>
#include <constants.h>
#include <regression.h>
#include <edge_detection.h>
#include <stats.hpp>
#include <sampling.h>
#include <filters.h>
#include <utils.hpp>


namespace 
{
  double sampling_distribution(double x)
  {
    const auto s = 0.05;
    const auto f = 0.5;
    const auto mf =  (1.0 - 2.0*f*s) / (1.0 - 2.0*s);

    if(x > 1.0) return 1.0;
    if(x < 0.0) return 0.0;
    if(x > (1 - s)) return (x-1.0)*f + 1.0;
    if(x < s) return f*x;
    else return (x - s)*mf + s*f;

  }
}


namespace cads
{

  z_type profile_decimate(z_type z, size_t width)
  {
    if(z.size() < width) return z;

    for (double i = 0; i < width; i++)
    {
      auto x = std::size_t(z.size() * sampling_distribution(i/width));
      z[std::size_t(i)] = z[x < z.size() ? x : z.size() - 1];
    }

    z.erase(z.begin()+width,z.end());
    return z;
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

  double average(z_type & v){
    if(v.empty()){
        return 0;
    }

    auto const count = static_cast<float>(v.size());
    return std::reduce(v.begin(), v.end()) / count;
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

/*
  coro<cads::msg,cads::msg,1> profile_alignment_coro(long long widthn, long long modulo, cads::Io<msg> &next)
  {
    cads::msg empty;
    GocatorProperties gp;

    for(long cnt = 0;;cnt++){
      
      auto [msg,terminate] = co_yield empty;  
      
      if(terminate) break;

      switch(std::get<0>(msg))
      {
        
        case msgid::scan: {

        }
        break;
        default:
          next.enqueue(msg);
      }
    }
  }  
*/

  z_type x_pos_skipNaN(const z_type &z, std::tuple<size_t,size_t> range)
  {
    if(z.size() < 1) {
      return z_type();
    }
    
    z_type rtn;

    for(size_t i = std::get<0>(range); i < std::get<1>(range);++i) {
      if(!std::isnan(z[i])) rtn.push_back(float(i));
    } 

    return rtn;
  }

  std::tuple<vector_NaN_free,vector_NaN_free> 
  extract_pulley_coords(const z_type &z, ConveyorProfile conveyor)
  {
    
    return {
      vector_NaN_free(conveyor.contains(ProfileSection::Left) ? 
        z_type(z.begin()+std::get<0>(conveyor[ProfileSection::Left]),z.begin()+std::get<1>(conveyor[ProfileSection::Left])) 
        : z_type())
      + 
      vector_NaN_free(conveyor.contains(ProfileSection::Right) ? 
        z_type(z.begin()+std::get<0>(conveyor[ProfileSection::Right]),z.begin()+std::get<1>(conveyor[ProfileSection::Right])) 
        : z_type()),

      vector_NaN_free(conveyor.contains(ProfileSection::Left) ?  x_pos_skipNaN(z,conveyor[ProfileSection::Left]) : z_type())
      + vector_NaN_free(conveyor.contains(ProfileSection::Right) ?  x_pos_skipNaN(z,conveyor[ProfileSection::Right]) : z_type())
    };

  }

  ConveyorProfile conveyor_profile_detection(const z_type &z, Dbscan config)
  {
    ConveyorProfile rtn = {};
    
    auto partition = (size_t)std::floor(z.size() / 2);

    z_type rz(z.rbegin() + partition, z.rend());
    auto clusters_left = dbscan(zrange{rz.begin(),rz.end()},config);
    auto clusters_right = dbscan(zrange{z.begin()+partition,z.end()},config);

    if(clusters_left.size() > 0 && clusters_right.size() > 0 )
    {
      auto left_edge = rz.size() - std::distance(rz.cbegin(),std::end(clusters_left[0]));
      auto right_edge = std::distance(z.cbegin(),std::end(clusters_right[0]));  
      rtn.insert({ProfileSection::Belt,{left_edge,right_edge}});
    }

    if(clusters_left.size() > 1 && clusters_right.size() > 0 )
    {
      auto left_edge = rz.size() - std::distance(rz.cbegin(),std::end(clusters_left[1]));
      auto right_edge = rz.size() - std::distance(rz.cbegin(),std::begin(clusters_left[1]));
      rtn.insert({ProfileSection::Left,{left_edge,right_edge}});
    }

    if(clusters_left.size() > 0 && clusters_right.size() > 1 )
    {
      auto left_edge = std::distance(z.cbegin(),std::begin(clusters_right[1]));  
      auto right_edge = std::distance(z.cbegin(),std::end(clusters_right[1]));  
      rtn.insert({ProfileSection::Right,{left_edge,right_edge}});
    }

    return rtn;
  }


} // namespace cads