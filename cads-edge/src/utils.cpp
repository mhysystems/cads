#include <utils.hpp>

#include <sstream>

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
}