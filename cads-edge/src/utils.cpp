#include <sstream>
#include <tuple>

#include <utils.hpp>


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
}