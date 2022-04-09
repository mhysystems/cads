#include <gtest/gtest.h>

#include <nan_removal.h>

using namespace cads;

TEST(cads, nan_removal)
{
  auto a = nan_removal({-1,0,2,2,2,1,-1},0);
  std::vector<int16_t> b{0,0,2,2,2,1,1};
  ASSERT_EQ(a,b);
}