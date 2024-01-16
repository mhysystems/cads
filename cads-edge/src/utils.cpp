#include <sstream>
#include <tuple>
#include <algorithm>
#include <numeric>
#include <fstream>
#include <ranges>

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

  msgid i_msgid(int i) {
    switch(i) {
      case 0 : return msgid::gocator_properties;
      case 1 : return msgid::scan;
      case 2 : return msgid::finished;
      case 3 : return msgid::begin_sequence;
      case 4 : return msgid::end_sequence;
      case 5 : return msgid::complete_belt;
      case 6 : return msgid::pulley_revolution_scan;
      case 7 : return msgid::stopped;
      case 8 : return msgid::nothing;
      case 9 : return msgid::select;
      case 10 : return msgid::caas_msg;
      case 11 : return msgid::measure;
      case 12 : return msgid::error;
      case 13 : return msgid::profile_partitioned;
    }
    return msgid::error;
  }
  
  coro<std::vector<float>,std::vector<float>,1> file_csv_coro(std::string filename) {
    
    using namespace std::ranges;

    std::ofstream file(filename);
    std::vector<float> len;
    len.push_back(0);

    while(true) {
      auto [data,terminate] = co_yield len;

      if(terminate) break;

      if(data.size() < 1) continue;
      
      for(auto v : data | views::drop(1)) {
        file << v << ',';
      }

      file << data.back() << '\n';
      file.flush();
      len[0] += (float)data.size();
    }

    co_return len;

  }
  
  coro<cads::msg,cads::msg,1> void_msg() {
    
    cads::msg empty;

    for(;;)
    {
      auto [msg,terminate] = co_yield empty;  
      if(terminate) break;
    }

    co_return empty;
  }
  
  std::vector<float> select_if(std::vector<float> a, std::vector<float> b, std::function<bool(float)> c)
  {
    for(size_t i = 0; i < a.size(); ++i)
    {
      a[i] = c(a[i]) ? b[i] : a[i];
    }
    return a;
  }

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

  std::string ReplaceString(std::string subject, const std::string &search,
                            const std::string &replace)
  {
    size_t pos = 0;
    while ((pos = subject.find(search, pos)) != std::string::npos)
    {
      subject.replace(pos, search.length(), replace);
      pos += replace.length();
    }
    return subject;
  }

  void write_vector(std::vector<double> xs,std::string name) {
    std::ofstream file{name};
    for(auto x : xs ) {
      file << x << '\n';
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

  std::function<double(double)> mk_online_mean() {
    auto existing_aggregate = std::make_tuple(0.0,0.0,0.0);

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

  std::array<size_t,2> minmin_element(const std::vector<double> & xs) {
    
    std::array<double,2> min = {std::numeric_limits<double>::max(),std::numeric_limits<double>::max()};
    std::array<size_t,2> mini = {xs.size(),xs.size()}; 

    size_t i = 0;
    for(auto x : xs) {
      if(x < min[1]) {
        min[1] = x;
        mini[1] = i;
      }
      
      if(min[0] > min[1]) {
        std::swap(min[0],min[1]);
        std::swap(mini[0],mini[1]);
      }

      ++i;
    }

    return mini;
  }
}