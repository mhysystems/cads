#include <iostream>
#include <chrono>
#include <thread>
#include <limits>
#include <queue>
#include <unordered_map>
#include <random>
#include <ranges>

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wuseless-cast" 
#pragma GCC diagnostic ignored "-Wshadow" 
#pragma GCC diagnostic ignored "-Wunused-parameter"
#pragma GCC diagnostic ignored "-Wconversion"
#pragma GCC diagnostic ignored "-Wnon-virtual-dtor"
#pragma GCC diagnostic ignored "-Wreorder"

#include <date/date.h>
#include <date/tz.h>
#include <fmt/core.h>
#include <fmt/chrono.h>
#include <spdlog/spdlog.h>
#include <cpr/cpr.h>
#include <nlohmann/json.hpp>
#include <z_data_generated.h>

#pragma GCC diagnostic pop

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

  auto send_flatbuffer_array(
      flatbuffers::FlatBufferBuilder &builder,
      double z_res,
      double z_off,
      std::vector<flatbuffers::Offset<CadsFlatbuffers::profile>> &profiles_flat,
      const cpr::Url &endpoint)
  {

    builder.Finish(CadsFlatbuffers::Createprofile_array(builder, z_res, z_off, profiles_flat.size(), builder.CreateVector(profiles_flat)));

    auto buf = builder.GetBufferPointer();
    auto size = builder.GetSize();

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

    return size;

  }

  void http_post_profile_properties_json(std::string json)
  {

    auto endpoint_url = global_config["upload_config_to"].get<std::string>();
    std::transform(endpoint_url.begin(), endpoint_url.end(), endpoint_url.begin(), [](unsigned char c)
                   { return std::tolower(c); });
    
    if (endpoint_url == "null") {
      spdlog::get("upload")->info("upload_config_to set to null. Config not uploaded");
      return;
    }

    cpr::Response r;
    const cpr::Url endpoint{endpoint_url};

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

  std::tuple<double,double,int>  http_post_profile_properties(std::string chrono)
  {
    nlohmann::json resolution;
    auto db_name = global_config["db_name"].get<std::string>();
    auto [y_resolution, x_resolution, z_resolution, z_offset, encoderResolution, z_max, z_min, err] = fetch_profile_parameters(db_name);
    
    if(err != 0) return {z_resolution,z_offset,err};

    resolution["site"] = global_config["site"].get<std::string>();
    resolution["conveyor"] = global_config["conveyor"].get<std::string>();
    resolution["chrono"] = chrono;
    resolution["y_res"] = y_resolution;
    resolution["x_res"] = x_resolution;
    resolution["z_res"] = z_resolution;
    resolution["z_off"] = z_offset;
    resolution["z_max"] = z_max;
    resolution["z_min"] = z_min;

    http_post_profile_properties_json(resolution.dump());
    return {z_resolution,z_offset,0};
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

    FlatBufferBuilder builder(4096 * 128);
    std::vector<flatbuffers::Offset<CadsFlatbuffers::profile>> profiles_flat;

    auto ts = fmt::format("{:%F-%H-%M}", std::chrono::system_clock::now());
    cpr::Url endpoint = {ReplaceString(global_config["upload_profile_to"].get<std::string>(), "%DATETIME%"s, ts)};
    
    auto [z_resolution,z_offset,err] = http_post_profile_properties(ts);
    
    if (err != 0)
    {
      spdlog::get("upload")->error("Unable to fetch profile parameters from DB");
      return 0;
    }

    auto start = std::chrono::high_resolution_clock::now();
    int64_t size = 0;

    while (true)
    {
      auto [co_terminate, cv] = fetch_profile.resume(0);
      auto [idx,p] = cv;

      if (!co_terminate)
      {
        namespace sr = std::ranges;
       
        auto tmp_z = p.z | sr::views::transform([=](float e) -> int16_t { return int16_t(((double)e - z_offset) / z_resolution);});
        std::vector<int16_t> short_z{tmp_z.begin(),tmp_z.end()};
        
        profiles_flat.push_back(CadsFlatbuffers::CreateprofileDirect(builder,p.y, p.x_off, &short_z));

        if (profiles_flat.size() == 256)
        {
          size += send_flatbuffer_array(builder, z_resolution, z_offset,profiles_flat, endpoint);
        }
      }
      else
      {
        if (profiles_flat.size() > 0)
        {
          size += send_flatbuffer_array(builder, z_resolution, z_offset, profiles_flat, endpoint);
        }
        break;
      }
    }
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::seconds>(end - start).count() + 1; // Add one second to avoid divide by zero

    spdlog::get("upload")->info("SIZE: {}, DUR:{}, RATE(Kb/s):{} ", size, duration, size / (1000*duration));
    spdlog::get("upload")->info("Leaving http_post_thread_bulk");
    return 0;
  }

}
