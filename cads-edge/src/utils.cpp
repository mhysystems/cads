#include <sstream>
#include <tuple>
#include <algorithm>
#include <numeric>

#include <utils.hpp>
#include <stats.hpp>


namespace {

  std::tuple<double,double,double> mean_update(std::tuple<double,double,double> existing_aggregate, double new_value) {
    
    auto [count,mean,M2] = existing_aggregate;
    count += 1;
    auto delta = new_value - mean;
    mean += delta / count;
    auto delta2 = new_value - mean;
    M2 += delta * delta2;

    return {count,mean,M2};
  }

}


namespace cads
{
  void dumpstack (lua_State *L) {
    int top=lua_gettop(L);
    for (int i=1; i <= top; i++) {
      printf("%d\t%s\t", i, luaL_typename(L,i));
      switch (lua_type(L, i)) {
        case LUA_TNUMBER:
          printf("%g\n",lua_tonumber(L,i));
          break;
        case LUA_TSTRING:
          printf("%s\n",lua_tostring(L,i));
          break;
        case LUA_TBOOLEAN:
          printf("%s\n", (lua_toboolean(L, i) ? "true" : "false"));
          break;
        case LUA_TNIL:
          printf("%s\n", "nil");
          break;
        default:
          printf("%p\n",lua_topointer(L,i));
          break;
      }
    }
  }




  std::string to_str(date::utc_clock::time_point c)
  {
    return date::format("%FT%TZ", c);
  }

  date::utc_clock::time_point to_clk(std::string s)
  {
    date::utc_clock::time_point datetime;
    std::istringstream (s) >> date::parse("%FT%TZ", datetime);

    return datetime;
  }

  std::function<double(double)> mk_online_mean(double mean) {
    auto existing_aggregate = std::make_tuple(1,mean,0);

    return [=](double new_value) mutable -> double {
      existing_aggregate = mean_update(existing_aggregate,new_value);
      return std::get<1>(existing_aggregate);
    };
  }

  double interquartile_mean(std::vector<float> &z) 
  {
    using namespace std::ranges;
    std::ranges::subrange<std::vector<float>::iterator> bbb;
    auto [q1,q3] = interquartile_range(z);
    auto quartile_filtered = z | views::filter([=](float a){ return a >= q1 && a <= q3;});
    auto sum = std::reduce(quartile_filtered.begin(),quartile_filtered.end());
    auto count = (double)std::ranges::distance(quartile_filtered.begin(),quartile_filtered.end());
    auto mean = sum / count;

    return mean;
  }

  double interquartile_mean(std::ranges::subrange<std::vector<float>::iterator>  z) 
  {
    using namespace std::ranges;
    auto [q1,q3] = interquartile_range(z,std::vector<float>{});
    auto quartile_filtered = z | views::filter([=](float a){ return a >= q1 && a <= q3;});
    auto sum = std::reduce(quartile_filtered.begin(),quartile_filtered.end());
    auto count = (double)std::ranges::distance(quartile_filtered.begin(),quartile_filtered.end());
    auto mean = sum / count;

    return mean;
  }


}