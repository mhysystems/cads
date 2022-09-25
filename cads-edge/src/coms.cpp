#include <chrono>
#include <ranges>
#include <algorithm>
#include <tuple>

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wuseless-cast"
#pragma GCC diagnostic ignored "-Wshadow"
#pragma GCC diagnostic ignored "-Wunused-parameter"
#pragma GCC diagnostic ignored "-Wconversion"
#pragma GCC diagnostic ignored "-Wnon-virtual-dtor"
#pragma GCC diagnostic ignored "-Wreorder"
#pragma GCC diagnostic ignored "-Wuseless-cast"

#include <fmt/core.h>
#include <fmt/chrono.h>
#include <spdlog/spdlog.h>
#include <cpr/cpr.h>
#include <nlohmann/json.hpp>
#include <z_data_generated.h>
#include <plot_data_generated.h>
#include <nats.h>
#include <blockingconcurrentqueue.h>
#include <coms.h>

#pragma GCC diagnostic pop


#include <constants.h>
#include <db.h>

using namespace std;

namespace cads
{
  moodycamel::BlockingConcurrentQueue<std::tuple<std::string,std::string>> nats_queue;

  void realtime_publish_thread(bool& terminate) {
    
    auto endpoint_url = global_config["nats_url"].get<std::string>();
    
    natsConnection  *conn  = nullptr;
    natsOptions *opts = nullptr;

    for(;!terminate;) {
      auto status = natsOptions_Create(&opts);
      if(status != NATS_OK) goto drop_msg;
      
      natsOptions_SetAllowReconnect(opts,true);
      natsOptions_SetURL(opts,endpoint_url.c_str());

      status = natsConnection_Connect(&conn, opts);
      if(status != NATS_OK) goto cleanup_opts;
      
      for(auto loop = true;loop && !terminate;) {
        std::tuple<std::string,std::string> msg;
        if(!nats_queue.wait_dequeue_timed(msg,std::chrono::milliseconds(1000))) {
          continue; // graceful thread terminate
        }

        auto s = natsConnection_PublishString(conn,get<0>(msg).c_str(),get<1>(msg).c_str());
        if(s == NATS_CONNECTION_CLOSED) {
          loop = false;
        }
      }

      natsConnection_Destroy(conn);
cleanup_opts:
      natsOptions_Destroy(opts);
drop_msg:
      if(!terminate) {
        std::tuple<std::string,std::string> dropped_msg;
        nats_queue.wait_dequeue(dropped_msg);
      }
    }

    nats_Close();
  }




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

  std::string mk_post_profile_url(std::string ts)
  {
    auto endpoint_url = global_config["upload_profile_to"].get<std::string>();

    if (endpoint_url == "null")
      return endpoint_url;

    auto site = global_config["site"].get<std::string>();
    auto conveyor = global_config["conveyor"].get<std::string>();

    return endpoint_url + '/' + site + '/' + conveyor + '/' + ts;
  }

  std::string mk_get_profile_url(double y, int len, std::string ts)
  {
    auto endpoint_url = global_config["upload_profile_to"].get<std::string>();

    if (endpoint_url == "null")
      return endpoint_url;

    auto site = global_config["site"].get<std::string>();
    auto conveyor = global_config["conveyor"].get<std::string>();

    return fmt::format("{}/{}-{}-{}/{}/{}", endpoint_url, site, conveyor, ts, y, len);
  }

  auto send_flatbuffer_array(
      flatbuffers::FlatBufferBuilder &builder,
      double z_res,
      double z_off,
      int64_t idx,
      std::vector<flatbuffers::Offset<CadsFlatbuffers::profile>> &profiles_flat,
      const cpr::Url &endpoint)
  {

    builder.Finish(CadsFlatbuffers::Createprofile_array(builder, z_res, z_off, idx, profiles_flat.size(), builder.CreateVector(profiles_flat)));

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


void http_post_realtime(double y_area, double value)
  {
    
    auto now = chrono::floor<chrono::seconds>(date::utc_clock::now()); // Default sends to much decimal precision for asp.net core
    auto ts = date::format("%FT%TZ", now);
    
    nlohmann::json params_json;
    params_json["Site"] = global_config["site"].get<std::string>();
    params_json["Conveyor"] = global_config["conveyor"].get<std::string>();
    params_json["Time"] = ts;
    params_json["YArea"] = y_area;
    params_json["Value"] = value;
    
    nats_queue.enqueue({"realtime",params_json.dump()});

  }

  void publish_meta_realtime(std::string Id, double value)
  {
    
    if(isnan(value) || isinf(value)) return;

    nlohmann::json params_json;
    params_json["Site"] = global_config["site"].get<std::string>();
    params_json["Conveyor"] = global_config["conveyor"].get<std::string>();
    params_json["Id"] = Id;
    params_json["Value"] = value;
    
    nats_queue.enqueue({"realtimemeta",params_json.dump()});
 
  }

  void http_post_profile_properties_json(std::string json)
  {

    auto endpoint_url = global_config["upload_config_to"].get<std::string>();

    if (endpoint_url == "null")
    {
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

  void http_post_profile_properties(int revid, std::string chrono)
  {
    nlohmann::json params_json;
    auto db_name = global_config["db_name"].get<std::string>();
    auto [params, err] = fetch_profile_parameters(db_name);
    auto [Ymax,YmaxN,WidthN,err2] = fetch_belt_dimensions(revid,db_name);

    params_json["site"] = global_config["site"].get<std::string>();
    params_json["conveyor"] = global_config["conveyor"].get<std::string>();
    params_json["chrono"] = chrono;
    params_json["y_res"] = params.y_res;
    params_json["x_res"] = params.x_res;
    params_json["z_res"] = params.z_res;
    params_json["z_off"] = params.z_off;
    params_json["z_max"] = params.z_max;
    params_json["Ymax"] = Ymax;
    params_json["YmaxN"] = YmaxN;
    params_json["WidthN"] = WidthN;

    if(err == 0 && err2 == 0) {
      http_post_profile_properties_json(params_json.dump());
    }
  }

  date::utc_clock::time_point http_post_whole_belt(int revid, int last_idx)
  {
    using namespace flatbuffers;

    auto now = chrono::floor<chrono::seconds>(date::utc_clock::now()); // Default sends to much decimal precision for asp.net core
    auto db_name = global_config["db_name"].get<std::string>();
    auto endpoint_url = global_config["upload_profile_to"].get<std::string>();
    auto y_max_length = global_config["y_max_length"].get<double>();

    if (endpoint_url == "null")
    {
      spdlog::get("upload")->info("http_post_whole_belt set to null. Belt not uploaded");
      return now;
    }

    auto fetch_profile = fetch_belt_coro(revid, last_idx);

    FlatBufferBuilder builder(4096 * 128);
    std::vector<flatbuffers::Offset<CadsFlatbuffers::profile>> profiles_flat;

    auto ts = date::format("%FT%TZ", now);
    cpr::Url endpoint{mk_post_profile_url(ts)};

    auto [params, err] = fetch_profile_parameters(db_name);
    
    if (err != 0)
    {
      spdlog::get("upload")->error("Unable to fetch profile parameters from DB");
      return now;
    }
    auto z_resolution = params.z_res;
    auto z_offset =  params.z_off;

    if(std::floor(y_max_length * 0.75 / params.y_res) > last_idx) {
      spdlog::get("upload")->error("Ignoring uploading incomplete belt with last idx of {}", last_idx);
      return now;
    }

    auto start = std::chrono::high_resolution_clock::now();
    int64_t size = 0;
    int64_t frame_idx = -1;
    double belt_z_max = 0; 
    int final_idx = 0;

    while (true)
    {
      auto [co_terminate, cv] = fetch_profile.resume(0);
      auto [idx, p] = cv;

      if (frame_idx < 0)
        frame_idx = (int64_t)idx;

      if (!co_terminate)
      {
        namespace sr = std::ranges;

        auto max_iter = max_element(p.z.begin(), p.z.end());
        
        if(max_iter != p.z.end()) {
          belt_z_max = max(belt_z_max, (double)*max_iter);
        }

        auto tmp_z = p.z | sr::views::transform([=](float e) -> int16_t
                                                { return NaN<float>::isnan(e) ? NaN<int16_t>::value : int16_t(((double)e - z_offset) / z_resolution); });
        std::vector<int16_t> short_z{tmp_z.begin(), tmp_z.end()};

        profiles_flat.push_back(CadsFlatbuffers::CreateprofileDirect(builder, p.y, p.x_off, &short_z));

        if (profiles_flat.size() == 256)
        {
          size += send_flatbuffer_array(builder, z_resolution, z_offset, frame_idx, profiles_flat, endpoint);
          frame_idx = -1;
        }
      }
      else
      {
        if (profiles_flat.size() > 0)
        {
          size += send_flatbuffer_array(builder, z_resolution, z_offset, frame_idx, profiles_flat, endpoint);
        }
        final_idx = idx;
        break;
      }
    }

    if(final_idx == last_idx-1) {
      http_post_profile_properties(revid,ts);
    }else{
      spdlog::get("upload")->error("Number of profiles sent {} not matching idx of {}", final_idx+1,last_idx);
    }

    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::seconds>(end - start).count() + 1; // Add one second to avoid divide by zero

    spdlog::get("upload")->info("ZMAX: {}, SIZE: {}, DUR:{}, RATE(Kb/s):{} ", belt_z_max, size, duration, size / (1000 * duration));
    spdlog::get("upload")->info("Leaving http_post_thread_bulk");
    return now;
  }

  std::vector<profile> http_get_frame(double y, int len, date::utc_clock::time_point chrono)
  {
    using namespace flatbuffers;

    auto ts = date::format("%Y-%m-%d-%H%M%OS", chrono);
    cpr::Url endpoint{mk_get_profile_url(y, len, ts)};
    cpr::Response r = cpr::Get(endpoint);

    auto pd = CadsFlatbuffers::Getplot_data(r.text.c_str());
    auto stride = pd->z_samples()->size() / pd->y_samples()->size();
    std::vector<profile> rtn;

    for (auto i = 0; auto e : *pd->y_samples())
    {

      auto z = pd->z_samples();
      rtn.push_back({e, pd->x_off(), {z->begin() + stride * i, z->begin() + stride * (1 + i++)}});
    }

    return rtn;
  }

}
