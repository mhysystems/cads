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
  z_type in{1, v, 1, v, 1, 1, 1, 1, v, 1, v, 1};
  spike_filter(in, 2);
  z_type out{1, v, v, v, 1, 1, 1, 1, v, v, v, 1};

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

TEST_F(CadsTest, upload_belt)
{
  using namespace std;

  ASSERT_EQ(create_db(), true);
  store_profile_parameters(1, 1, 1, 1, 1, 1, 0);
  auto store_profile = store_profile_coro();

  profile p;

  p = {0, 0, {1, 2, 3}};
  store_profile.resume({0, 0, p});

  p = {1, 0, {4, 5, 6}};
  store_profile.resume({0, 1, p});

  auto ts = http_post_whole_belt(0, 2);
  auto rst = http_get_frame(0, 2, ts);

  p = {0, 0, {1, 2, 3}};
  ASSERT_EQ(rst[0], p);

  p = {1, 0, {4, 5, 6}};
  ASSERT_EQ(rst[1], p);
}
