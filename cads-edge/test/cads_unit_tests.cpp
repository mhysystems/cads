#include <fstream>

#include <gtest/gtest.h>

#include <edge_detection.h>
#include <db.h>
#include <filters.h>
#include <constants.h>
#include <coms.h>
#include <init.h>

using namespace cads;

class CadsTest : public ::testing::Test
{
protected:
  void SetUp() override
  {
    init_config("test/tconfig.json");
    if (global_config.find("log_lengthMiB") != global_config.end())
    {
      init_logs(global_config["log_lengthMiB"].get<size_t>() * 1024 * 1024, 60);
    }
    else
    {
      init_logs(5 * 1024 * 1024, 60);
    }
  }

  void TearDown() override
  {
    drop_config();
  }
};

TEST(cads, nan_filter)
{
  auto n = NaN<z_element>::value;
  std::vector<z_element> a = {1, n, 2, n, 2, 1, n};
  nan_filter(a);
  std::vector<z_element> b{1, 1, 2, 2, 2, 1, 1};
  ASSERT_EQ(a, b);
}

TEST(cads, find_profile_edges_nans_outer)
{
  auto n = NaN<z_type::value_type>::value;

  auto a = find_profile_edges_nans_outer({1, 1, n, n, n, n, n, 1, 2, 2, 2, 1, n, n, n, n, n, 1, 1, 1}, 3);
  std::tuple<int, int> b{7, 12};
  ASSERT_EQ(a, b);
}

TEST(cads, spike_filter)
{
  auto v = NaN<z_type::value_type>::value;
  z_type in{1, v, 1, v, 1, 1, 1, v, 1, 1, 1, 1, v, 1, v, 1};
  spike_filter(in, 5);
  z_type out{1, v, v, v, v, v, v, v, 1, 1, 1, 1, v, v, v, 1};

  ASSERT_EQ(in.size(), out.size());

  auto same = true;
  for (size_t i = 0; i < in.size(); ++i)
  {
    if (NaN<z_type::value_type>::isnan(in[i]))
    {
      same = same && NaN<z_type::value_type>::isnan(out[i]);
    }
    else
    {
      same = same && (in[i] == out[i]);
    }
  }

  ASSERT_EQ(same, true);

  in = {};
  spike_filter(in, 3);
  out = {};
  ASSERT_EQ(in, out);
}

TEST_F(CadsTest, spike_filter2)
{
  auto fetch_profile = fetch_belt_coro(0, std::numeric_limits<int>::max(), 256, "test/demo.db");
  const int nan_num = global_config["left_edge_nan"].get<int>();
  const int spike_window_size = 25;
  create_db("spike_filter2.db");
  auto store_profile = store_profile_coro("spike_filter2.db");
  
  while (true)
  {

    auto [co_terminate, cv] = fetch_profile.resume(0);
    auto [idx, p] = cv;
    //spike_filter(p.z, nan_num/4);
    spike_filter(p.z, 25);
    //spike_filter(p.z, spike_window_size);
    store_profile.resume({0, idx, p});

    if (co_terminate)
    {
      break;
    }
  }
}

#if 0
TEST_F(CadsTest, upload_belt)
{
  using namespace std;

  ASSERT_EQ(create_db(), true);
  store_profile_parameters({0.5, 0.5, 0.5, 0.5, 0.5, 30});
  auto store_profile = store_profile_coro();

  profile p;

  p = {0, -2, {30, 30, 30, 30, 30}};
  store_profile.resume({0, 0, p});

  p = {0.5, -2, {30, 27, 27, 27, 30}};
  store_profile.resume({0, 1, p});

  p = {1, -2, {30, 27, 25, 27, 30}};
  store_profile.resume({0, 2, p});

  p = {1.5, -2, {30, 27, 27, 27, 30}};
  store_profile.resume({0, 3, p});

  p = {2, -2, {30, 30, 30, 30, 30}};
  store_profile.resume({0, 4, p});


  auto ts = http_post_whole_belt(0, 5);
  auto rst = http_get_frame(0, 5, ts);

  p = {0, -2, {30, 30, 30, 30, 30}};
  ASSERT_EQ(rst[0], p);

  p = {0.5, -2, {30, 27, 27, 27, 30}};
  ASSERT_EQ(rst[1], p);

  p = {1, -2, {30, 27, 25, 27, 30}};
  ASSERT_EQ(rst[2], p);

  p = {1.5, -2, {30, 27, 27, 27, 30}};
  ASSERT_EQ(rst[3], p);

  p = {2, -2, {30, 30, 30, 30, 30}};
  ASSERT_EQ(rst[4], p);
}
#endif
