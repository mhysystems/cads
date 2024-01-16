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
#include <system_error>
#include <expected>

#include <profile.h>
#include <constants.h>
#include <regression.h>
#include <stats.hpp>
#include <sampling.h>
#include <filters.h>
#include <utils.hpp>


namespace 
{

template<class T> std::vector<T> delta_coding(std::vector<T> zs) requires std::integral<T> || std::floating_point<T>
{
	T z_n = 0;
	for(auto& z : zs)
	{
		auto z_tmp = z;
		z = z - z_n;
		z_n = z_tmp;
	}

	return zs;
}

template<class T> std::vector<T> delta_decoding(std::vector<T> zs) requires std::integral<T> || std::floating_point<T>
{
	T z_n = 0;
	for(auto& z : zs)
	{
		z = z + z_n;
		z_n = z;
	}

	return zs;
}

std::vector<int> quantise(cads::z_type zs, float res)
{
	std::vector<int> r;
	for(auto z : zs) {
		r.push_back(!std::isnan(z) ? (int)(z / res) : std::numeric_limits<int>::lowest());
	}
 
	return r;
}

cads::z_type unquantise(std::vector<int> zs, cads::z_element res)
{
	cads::z_type r;
	for(auto z : zs) {
		r.push_back(z != std::numeric_limits<int>::lowest() ? z * res : std::numeric_limits<cads::z_element>::quiet_NaN());
	}
	return r;
}

cads::z_type zbitpacking(const std::vector<int> bs, int s, int min) {
	
  cads::z_type r;
  r.push_back(std::bit_cast<cads::z_element>((int)bs.size()));
  r.push_back(std::bit_cast<float>(s));
  r.push_back(std::bit_cast<float>(min));
 
	auto mask = (1 << s) - 1 ;
	
	int p = 0;
	auto bi = bs.cbegin();
  r.push_back(std::bit_cast<cads::z_element>(*bi++));
  int br = 0;
  while (bi < bs.cend()) {
    int i = br;
    unsigned int v = 0;
    
    for(;i < sizeof(int)*CHAR_BIT && bi < bs.cend(); i+=s) {
      v = *bi++ - min;
      p |= (v & mask) << i;
    }
    
    r.push_back(std::bit_cast<cads::z_element>(p));
    br = i - sizeof(int)*CHAR_BIT;
    
    p = (v & mask) >> (s-br) ;
  }
  if(br > 0) r.push_back(std::bit_cast<cads::z_element>(p));
  return r;
	
}

std::vector<int> zbitunpacking(const cads:: z_type bs) {
	
  auto bi = bs.cbegin();
  auto len = std::bit_cast<int>(*bi++);
  auto s = std::bit_cast<int>(*bi++);
  auto min = std::bit_cast<int>(*bi++);
  auto mask = (1 << s) - 1 ;

  std::vector<int> r(len,0);
  
  size_t c = 0;
  r[c++] = std::bit_cast<int>(*bi++);
  int i = 0;
  int l = 0;
  int sr = 0;
  const int max_bits = sizeof(int)*CHAR_BIT;
  
  
  while (c < len) {
    
    const int v = std::bit_cast<int>(*bi);
    r[c] |= ((((unsigned int)v  << l ) >> sr) & mask);

    i += s;
    const int lmax = i <= max_bits;
    const int gmax = i >= max_bits;

    r[c] += min*lmax;
    c += lmax;
    bi += gmax;
    l = (max_bits - i + s)*(i > max_bits);
    sr = i*lmax;

    i = i - max_bits*gmax - s*(l > 0);
  }

  return r;
}

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

  z_type x_pos_skipNaN(const z_type &z, std::tuple<size_t,size_t> range)
  {
    if(z.size() < 1) {
      return z_type();
    }
    
    z_type rtn;

    for(size_t i = std::get<0>(range); i < std::get<1>(range);++i) {
      if(!std::isnan(z[i])) rtn.push_back(z_type::value_type(i));
    } 

    return rtn;
  }

  z_type extract_partition(const ProfilePartitioned& profile, ProfileSection section)
  {
    if(profile.partitions.contains(section)) {
      return z_type(profile.scan.z.begin()+std::get<0>(profile.partitions.at(section)),profile.scan.z.begin()+std::get<1>(profile.partitions.at(section)));
    }else {
      return z_type();
    }
  }

  std::tuple<vector_NaN_free,vector_NaN_free> 
  extract_pulley_coords(const z_type &z, ProfilePartitions conveyor)
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

  std::tuple<vector_NaN_free,vector_NaN_free> 
  extract_belt_coords(const z_type &z, ProfilePartitions conveyor)
  {
    return {
      vector_NaN_free(conveyor.contains(ProfileSection::Belt) ? 
        z_type(z.begin()+std::get<0>(conveyor[ProfileSection::Belt]),z.begin()+std::get<1>(conveyor[ProfileSection::Belt])) 
        : z_type()),
      vector_NaN_free(conveyor.contains(ProfileSection::Belt) ?  x_pos_skipNaN(z,conveyor[ProfileSection::Belt]) : z_type())
    };
  }


  size_t distance(std::tuple<size_t, size_t> x)
  {
    return std::get<1>(x) > std::get<0>(x) ? std::get<1>(x) - std::get<0>(x) : std::get<0>(x) - std::get<1>(x);
  }

  ProfilePartitions conveyor_profile_detection(const profile &profile, Dbscan config)
  {
    ProfilePartitions rtn = {};
    auto& z = profile.z;
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
  
  profile unpackzbits(profile p,double res)
  {
    auto ubp = ::zbitunpacking(p.z);
    auto dc = delta_decoding(ubp) ; 
    auto zs = unquantise(dc,(float)res);

    return {p.time,p.y,p.x_off,zs};
  }

  profile packzbits(profile p,double res)
  {
    auto zs_dc_q = quantise(p.z,(float)res) ; 
    auto zs_dc = delta_coding(zs_dc_q) ; 
	  
	  const auto [min,max] = std::ranges::minmax_element(zs_dc | std::views::drop(1));

	  auto l = (int)ceil(log((double)*max - (double)*min) / log(2.0));
    auto bp = ::zbitpacking(zs_dc,l,*min);

    return {p.time,p.y,p.x_off,bp};
  }
 
} // namespace cads