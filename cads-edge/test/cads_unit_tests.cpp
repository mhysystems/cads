#include <gtest/gtest.h>

#include <nan_removal.h>
#include <fiducial.h>

using namespace cads;

TEST(cads, make_fiducial)
{
  auto f = make_fiducial(0.7,5.5,50,50,50);
  auto w = fiducial_as_image(f);
  ASSERT_EQ(w,true);

}

TEST(cads, nan_removal)
{
  auto a = nan_removal({-1,0,2,2,2,1,-1},0);
  std::vector<int16_t> b{0,0,2,2,2,1,1};
  ASSERT_EQ(a,b);
}

