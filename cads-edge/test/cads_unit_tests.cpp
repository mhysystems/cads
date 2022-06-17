#include <gtest/gtest.h>

#include <edge_detection.h>
#include <nan_removal.h>
#include <fiducial.h>
#include <db.h>
#include <window.hpp>
#include <cmath>

#include <filters.h>
#include <constants.h>

#include <dynamic_processing.h>

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

TEST(cads, find_profile_edges_nans_outer)
{
  auto n = NaN<z_type::value_type>::value;

  auto a = find_profile_edges_nans_outer({1,1,n,n,n,n,n,1,2,2,2,1,n,n,n,n,n,1,1,1},3);
  std::tuple<int,int> b{7,12};
  ASSERT_EQ(a,b);
}

TEST(cads, dynamic_processing)
{
  profile p;
  auto proc = lua_processing_coro();
  auto [e,a] = proc(p);
  ASSERT_EQ(a,9);
}


TEST(cads, search_for_fiducial)
{
#if 0
  const auto max_y = 169888; 
  auto y_res = 5.5758;
  auto x_res = 0.795;
  auto fiducial = make_fiducial(x_res,y_res,50,75,50);
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
    auto a = search_for_fiducial(window_cv,fiducial,30.0);
    if(a < b) {
      b = a;
      yy = y;
      fiducial_as_image(window_cv);
    }

    belt_window.pop_front();
  
  }
  
  close_db(db,ignore);
  ASSERT_EQ(b,0.0) << yy << '\n';
#endif
}


TEST(cads, search_for_fiducial16bitCorr)
{
#if 0
  const auto max_y = 169888; 
  auto y_res = 5.5758;
  auto x_res = 0.795;
  auto fiducial = make_fiducial(x_res,y_res,50,75,50);
  cads::window belt_window;


  auto [db,ignore] = open_db("profile.db");
  auto fetch_stmt = fetch_profile_statement(db);
  double b = 0.2;
  int yy = 0;
  for(auto y = 60000;y < 60443+fiducial.rows;y++) {
    auto profile = fetch_profile(fetch_stmt,y);
    
    belt_window.push_back(profile);

    // Wait for buffers to fill
    if (belt_window.size() < fiducial.rows) continue;

    auto window_cv = window_to_mat(belt_window,x_res);
    auto a = search_for_fiducial(window_cv,fiducial,30.0);
    if(a < b) {
      b = a;
      yy = y;
      fiducial_as_image(window_cv);
    }

    belt_window.pop_front();
  
  }
  
  close_db(db,ignore);
  ASSERT_EQ(b,0.0) << yy << '\n';
#endif
}

TEST(cads, spike_filter)
{
  auto v = NaN<z_type::value_type>::value;
  z_type in{1,v,1,1,v,1};
  spike_filter(in,3);
  z_type out{1,v,v,v,v,1};
  ASSERT_EQ(in,out);

}




