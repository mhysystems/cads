#pragma once

#include <string>
#include <vector>

#include <profile.h>

namespace cads
{
  std::string http_post_whole_belt(int, int);
  std::vector<profile> http_get_frame(double y, int len, std::string ts);
}
