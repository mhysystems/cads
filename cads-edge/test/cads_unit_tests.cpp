#include <gtest/gtest.h>

#include <nan_removal.h>
#include <fiducial.h>
#include <db.h>
#include <window.hpp>
#include <cmath>

using namespace cads;

TEST(cads, make_fiducial)
{
  auto f = make_fiducial(0.795,5.5758,50,50,50);
  auto w = fiducial_as_image(f);
  ASSERT_EQ(w,true);

}

TEST(cads, nan_removal)
{
  auto a = nan_removal({-1,0,2,2,2,1,-1},0);
  std::vector<int16_t> b{0,0,2,2,2,1,1};
  ASSERT_EQ(a,b);
}


TEST(cads, search_for_fiducial)
{
  const auto max_y = 169888; 
  auto y_res = 5.5758;
  auto x_res = 0.795;
  auto fiducial = make_fiducial(x_res,y_res,50,50,50);
  cads::window belt_window;


  auto [db,ignore] = open_db("testbelt.db");
  auto fetch_stmt = fetch_profile_statement(db);
  double b = 0.2;
  int yy = 0;
  for(auto y = 60000;y < 60443+fiducial.rows;y++) {
    auto profile = fetch_profile(fetch_stmt,y);
    belt_window.push_back(profile);

    // Wait for buffers to fill
    if (belt_window.size() < fiducial.rows) continue;

    auto window_cv = window_to_mat(belt_window,x_res);
    auto a = search_for_fiducial(window_cv,fiducial,0.0,30.0);
    if(a < b) {
      b = a;
      yy = y;
      fiducial_as_image(window_cv);
    }

    belt_window.pop_front();
  
  }

  close_db(db,ignore);
  ASSERT_EQ(b,0.0) << yy << '\n';
}



