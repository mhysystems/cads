#include <iostream>
#include <chrono>
#include <thread>
#include <limits>
#include <queue>
#include <unordered_map>
#include <random>

#include <date/date.h>
#include <date/tz.h>
#include <fmt/core.h>
#include <fmt/chrono.h>
#include <spdlog/spdlog.h>
#include <cpr/cpr.h>
#include <nlohmann/json.hpp>

#include <z_data_generated.h>

#include <constants.h>
#include <db.h>


using namespace std;

namespace cads
{

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

  void send_flatbuffer_array(
      flatbuffers::FlatBufferBuilder &builder,
      std::vector<flatbuffers::Offset<cads_flatworld::profile>> &profiles_flat,
      const cpr::Url &endpoint)
  {

    builder.Finish(cads_flatworld::Createprofile_array(builder, profiles_flat.size(), builder.CreateVector(profiles_flat)));

    auto buf = builder.GetBufferPointer();
    auto size = builder.GetSize();
    auto start = std::chrono::high_resolution_clock::now();

    while (true)
    {
      cpr::Response r = cpr::Post(endpoint,
                                  cpr::Body{(char *)buf, size},
                                  cpr::Header{{"Content-Type", "application/octet-stream"}});

      if (cpr::ErrorCode::OK == r.error.code && cpr::status::HTTP_OK == r.status_code)
      {
        break;
      }
      else
      {
        spdlog::get("upload")->error("Upload failed with http status code {}", r.status_code);
      }
    }

    builder.Clear();
    profiles_flat.clear();
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start).count() / 1000.0;

    spdlog::get("upload")->info("SIZE: {}, DUR:{}, RATE(Kb/s):{} ", size, duration, (double)size / duration);
  }

  void http_post_profile_properties(std::string json, std::string ts)
  {

    auto endpoint_url = global_config["upload_profile_to"].get<std::string>();
    std::transform(endpoint_url.begin(), endpoint_url.end(), endpoint_url.begin(), [](unsigned char c)
                   { return std::tolower(c); });
    if (endpoint_url == "null")
      return;

    cpr::Response r;
    const cpr::Url endpoint{ReplaceString(global_config["upload_config_to"].get<std::string>(), "%DATETIME%"s, ts)};

    while (true)
    {
      r = cpr::Post(endpoint,
                    cpr::Body{json},
                    cpr::Header{{"Content-Type", "application/json"}});

      if (cpr::ErrorCode::OK == r.error.code && cpr::status::HTTP_OK == r.status_code)
      {
        break;
      }
      else
      {
        spdlog::get("upload")->error("First Upload failed with http status code {}", r.status_code);
      }
    }
  }

  void http_post_profile_properties(double y_resolution, double x_resolution, double z_resolution, double z_offset, std::string ts)
  {
    nlohmann::json resolution;

    resolution["y"] = y_resolution;
    resolution["x"] = x_resolution;
    resolution["z"] = z_resolution;
    resolution["z_off"] = z_offset;

    http_post_profile_properties(resolution.dump(), ts);
  }


  int http_post_whole_belt(int revid,int last_idx)
  {
    using namespace flatbuffers;

    auto db_name = global_config["db_name"].get<std::string>();
    auto endpoint_url = global_config["upload_profile_to"].get<std::string>();
    std::transform(endpoint_url.begin(), endpoint_url.end(), endpoint_url.begin(), [](unsigned char c)
                   { return std::tolower(c); });

    if (endpoint_url == "null")
      return 0;

    auto fetch_profile = fetch_belt_coro(revid,last_idx);

    auto start = std::chrono::high_resolution_clock::now();

    FlatBufferBuilder builder(4096 * 128);
    std::vector<flatbuffers::Offset<cads_flatworld::profile>> profiles_flat;

    auto ts = fmt::format("{:%F-%H-%M}", std::chrono::system_clock::now());
    cpr::Url endpoint = {ReplaceString(global_config["upload_profile_to"].get<std::string>(), "%DATETIME%"s, ts)};
    auto [y_resolution, x_resolution, z_resolution, z_offset, encoderResolution, err] = fetch_profile_parameters(db_name);
    
    if (err == 0)
    {
      http_post_profile_properties(y_resolution, x_resolution, z_resolution, z_offset, ts);
    }
    else
    {
      spdlog::get("upload")->error("Unable to fetch profile parameters from DB");
      return 0;
    }

    while (true)
    {
      auto [co_terminate, cv] = fetch_profile(0);
      auto [idx,p] = cv;

      if (!co_terminate)
      {
        profiles_flat.push_back(cads_flatworld::CreateprofileDirect(builder, idx, p.y, p.x_off, &p.z));

        if (profiles_flat.size() == 256)
        {
          spdlog::get("upload")->info("Send Array {}", endpoint.str());
          send_flatbuffer_array(builder, profiles_flat, endpoint);
        }
      }
      else
      {
        if (profiles_flat.size() > 0)
        {
          spdlog::get("upload")->info("Last Array {}", endpoint.str());
          send_flatbuffer_array(builder, profiles_flat, endpoint);
        }
        break;
      }
    }

    spdlog::get("upload")->info("Leaving http_post_thread_bulk");
    return 0;
  }

}
