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
#include <coms.h>
#include <brotli/encode.h>

#pragma GCC diagnostic pop

#include <constants.h>
#include <utils.hpp>

using namespace std;

namespace
{

  std::tuple<std::string, bool> http_post_json(std::string endpoint_url, std::string json, int retry = 16, int retry_wait = 8)
  {

    cpr::Response r;
    const cpr::Url endpoint{endpoint_url};

    while (retry-- > 0)
    {
      r = cpr::Post(endpoint,
                    cpr::Body{json},
                    cpr::Header{{"Content-Type", "application/json"}});

      if (cpr::ErrorCode::OK == r.error.code && cpr::status::HTTP_OK == r.status_code)
      {
        return {r.text, false};
      }
      else
      {
        if (retry == 0)
        {
          spdlog::get("upload")->error("http_post_json failed with http status code {}, body: {}, endpoint: {}", r.status_code, json, endpoint.str());
        }
        else
        {
          std::this_thread::sleep_for(std::chrono::seconds(retry_wait));
        }
      }
    }

    return {"", true};
  }

  std::vector<uint8_t> compress(uint8_t *input, uint32_t size) {

    std::vector<uint8_t> output(size);
    
    size_t input_size = size;
    size_t output_size = output.size();

    BrotliEncoderCompress(8,BROTLI_DEFAULT_WINDOW, BROTLI_DEFAULT_MODE, input_size, input, &output_size, output.data());
    output.resize(output_size);
    return output;

  }
}

namespace cads
{
  moodycamel::BlockingConcurrentQueue<std::tuple<std::string, std::string>> nats_queue;

  void remote_control_thread(moodycamel::BlockingConcurrentQueue<int> &nats_queue, bool &terminate)
  {

    auto endpoint_url = communications_config.NatsUrl;

    natsConnection *conn = nullptr;
    natsOptions *opts = nullptr;
    natsSubscription *sub = nullptr;

    for (; !terminate;)
    {
      auto status = natsOptions_Create(&opts);
      if (status != NATS_OK)
        goto drop_msg;

      natsOptions_SetAllowReconnect(opts, true);
      natsOptions_SetURL(opts, endpoint_url.c_str());

      status = natsConnection_Connect(&conn, opts);
      if (status != NATS_OK)
        goto cleanup_opts;

      status = natsConnection_SubscribeSync(&sub, conn, "originReset");
      if (status != NATS_OK)
        goto cleanup_opts;

      for (auto loop = true; loop && !terminate;)
      {

        natsMsg *msg = nullptr;
        auto s = natsSubscription_NextMsg(&msg, sub, 500);

        if (s == NATS_TIMEOUT)
        {
        }
        else if (s == NATS_CONNECTION_CLOSED)
        {
          loop = false;
        }
        else if (s == NATS_OK)
        {
          std::string sub(natsMsg_GetSubject(msg));
          std::string msg_string(natsMsg_GetData(msg), natsMsg_GetDataLength(msg));
          natsMsg_Destroy(msg);
          nats_queue.enqueue(0);
        }
      }
    }

    natsConnection_Destroy(conn);
  cleanup_opts:
    natsOptions_Destroy(opts);
  drop_msg:
    nats_Close();
  }

  void realtime_publish_thread(bool &terminate)
  {

    auto endpoint_url = communications_config.NatsUrl;

    natsConnection *conn = nullptr;
    natsOptions *opts = nullptr;

    for (; !terminate;)
    {
      auto status = natsOptions_Create(&opts);
      if (status != NATS_OK)
        goto drop_msg;

      natsOptions_SetAllowReconnect(opts, true);
      natsOptions_SetURL(opts, endpoint_url.c_str());

      status = natsConnection_Connect(&conn, opts);
      if (status != NATS_OK)
        goto cleanup_opts;

      for (auto loop = true; loop && !terminate;)
      {
        std::tuple<std::string, std::string> msg;
        if (!nats_queue.wait_dequeue_timed(msg, std::chrono::milliseconds(1000)))
        {
          continue; // graceful thread terminate
        }

        auto s = natsConnection_PublishString(conn, get<0>(msg).c_str(), get<1>(msg).c_str());
        if (s == NATS_CONNECTION_CLOSED)
        {
          loop = false;
        }
      }

      natsConnection_Destroy(conn);
    cleanup_opts:
      natsOptions_Destroy(opts);
    drop_msg:
      if (!terminate)
      {
        std::tuple<std::string, std::string> dropped_msg;
        if (nats_queue.wait_dequeue_timed(dropped_msg, std::chrono::seconds(1)))
        {
          goto drop_msg;
        }
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

  std::tuple<int, bool> remote_addconveyor(Conveyor params)
  {
    auto [url, enable] = global_webapi.add_conveyor;

    if (enable)
    {
      auto [text, err] = http_post_json(url, params);
      auto json = nlohmann::json::parse(text, nullptr, false);

      if (json.is_number_integer())
      {
        auto conveyor_id = json.get<int>();
        return {conveyor_id, err};
      }
      else
      {
        return {0, true};
      }
    }
    else
    {
      return {0, true};
    }
  }

  std::tuple<int, bool> remote_addbelt(Belt params)
  {
    auto [url, enable] = global_webapi.add_belt;

    if (enable)
    {
      std::string ps = params;
      auto [text, err] = http_post_json(url, ps);
      auto json = nlohmann::json::parse(text, nullptr, false);

      if (json.is_number_integer())
      {
        auto conveyor_id = json.get<int>();
        return {conveyor_id, err};
      }
      else
      {
        return {0, true};
      }
    }
    else
    {
      return {0, true};
    }
  }

  std::string mk_post_profile_url(std::string ts)
  {
    auto endpoint_url = global_config["base_url"].get<std::string>() + "/belt";

    if (endpoint_url == "null")
      return endpoint_url;

    auto site = global_conveyor_parameters.Site;
    auto conveyor = global_conveyor_parameters.Name;

    return endpoint_url + '/' + site + '/' + conveyor + '/' + ts;
  }

  std::string mk_post_profile_url(std::string endpoint_url, std::string ts)
  {
    auto site = global_conveyor_parameters.Site;
    auto conveyor = global_conveyor_parameters.Name;

    return endpoint_url + '/' + site + '/' + conveyor + '/' + ts;
  }

  std::string mk_get_profile_url(double y, int len, std::string ts)
  {
    auto endpoint_url = global_config["base_url"].get<std::string>() + "/belt";

    if (endpoint_url == "null")
      return endpoint_url;

    auto site = global_conveyor_parameters.Site;
    auto conveyor = global_conveyor_parameters.Name;

    return fmt::format("{}/{}-{}-{}/{}/{}", endpoint_url, site, conveyor, ts, y, len);
  }

  auto send_flatbuffer_array(
      flatbuffers::FlatBufferBuilder &builder,
      double z_res,
      double z_off,
      int64_t idx,
      std::vector<flatbuffers::Offset<CadsFlatbuffers::profile>> &profiles_flat,
      const cpr::Url &endpoint,
      bool upload_profile = true)
  {

    builder.Finish(CadsFlatbuffers::Createprofile_array(builder, z_res, z_off, idx, profiles_flat.size(), builder.CreateVector(profiles_flat)));

    auto buf = builder.GetBufferPointer();
    auto size = builder.GetSize();

    std::vector<uint8_t> bufv = compress(buf,size);


    while (upload_profile)
    {
      cpr::Response r = cpr::Post(endpoint,
                                  cpr::Body{(char *)bufv.data(), bufv.size()},
                                  cpr::Header{{"Content-Encoding", "br"},{"Content-Type", "application/octet-stream"}});

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
    params_json["Site"] = global_conveyor_parameters.Site;
    params_json["Conveyor"] = global_conveyor_parameters.Name;
    params_json["Time"] = ts;
    params_json["YArea"] = y_area;
    params_json["Value"] = value;

    nats_queue.enqueue({"/realtime", params_json.dump()});
  }

  void publish_meta_realtime(std::string Id, double value, bool valid)
  {

    if (isnan(value) || isinf(value))
      return;

    nlohmann::json params_json;
    params_json["Site"] = global_conveyor_parameters.Site;
    params_json["Conveyor"] = global_conveyor_parameters.Name;
    params_json["Id"] = Id;
    params_json["Value"] = value;
    params_json["Valid"] = valid;

    nats_queue.enqueue({"/realtimemeta", params_json.dump()});
  }

  void http_post_profile_properties_json(std::string json)
  {

    auto endpoint_url = global_config["base_url"].get<std::string>() + "/meta";
    auto upload_props = global_config["upload_profile"].get<bool>();

    if (endpoint_url == "null")
    {
      spdlog::get("upload")->info("base_url set to null. Config not uploaded");
      return;
    }

    cpr::Response r;
    const cpr::Url endpoint{endpoint_url};

    while (upload_props)
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
        spdlog::get("upload")->error("http_post_profile_properties_json failed with http status code {}, body: {}, endpoint: {}", r.status_code, json, endpoint.str());
      }
    }
  }

  void http_post_profile_properties(int revid, int last_idx, std::string chrono, double belt_length, double y_step)
  {
    nlohmann::json params_json;
    auto db_name = global_config["profile_db_name"].get<std::string>();
    auto [params, err] = fetch_profile_parameters(db_name);
    auto [Ymin, Ymax, YmaxN, WidthN, err2] = fetch_belt_dimensions(revid, last_idx, db_name);
    auto [belt_id, err3] = fetch_belt_id();

    params_json["site"] = global_conveyor_parameters.Site;
    params_json["conveyor"] = global_conveyor_parameters.Name;
    params_json["chrono"] = chrono;
    params_json["y_res"] = y_step;
    params_json["x_res"] = params.x_res;
    params_json["z_res"] = params.z_res;
    params_json["z_off"] = params.z_off;
    params_json["z_max"] = params.z_max;
    params_json["Ymax"] = belt_length;
    params_json["YmaxN"] = YmaxN;
    params_json["WidthN"] = WidthN;
    params_json["Belt"] = belt_id;

    if (err == 0 && err2 == 0)
    {
      http_post_profile_properties_json(params_json.dump());
    }
  }

  void http_post_profile_properties2(std::string chrono, double YmaxN, double y_step)
  {
    nlohmann::json params_json;
    auto db_name = global_config["profile_db_name"].get<std::string>();
    auto [params, err] = fetch_profile_parameters(db_name);
    auto [belt_id, err2] = fetch_belt_id();

    params_json["site"] = global_conveyor_parameters.Site;
    params_json["conveyor"] = global_conveyor_parameters.Name;
    params_json["chrono"] = chrono;
    params_json["y_res"] = y_step;
    params_json["x_res"] = params.x_res;
    params_json["z_res"] = params.z_res;
    params_json["z_off"] = params.z_off;
    params_json["z_max"] = params.z_max;
    params_json["Ymax"] = global_belt_parameters.Length;
    params_json["YmaxN"] = YmaxN;
    params_json["WidthN"] = global_config["width_n"].get<double>();
    params_json["Belt"] = belt_id;

    if (err == 0 && err2 == 0)
    {
      http_post_profile_properties_json(params_json.dump());
    }
  }

  void http_post_profile_properties(cads::meta meta)
  {
    nlohmann::json params_json;

    params_json["site"] = meta.site;
    params_json["conveyor"] = meta.conveyor;
    params_json["chrono"] = meta.chrono;
    params_json["y_res"] = meta.y_res;
    params_json["x_res"] = meta.x_res;
    params_json["z_res"] = meta.z_res;
    params_json["z_off"] = meta.z_off;
    params_json["z_max"] = meta.z_max;
    params_json["z_min"] = meta.z_min;
    params_json["Ymax"] = meta.Ymax;
    params_json["YmaxN"] = meta.YmaxN;
    params_json["WidthN"] = meta.WidthN;


    http_post_profile_properties_json(params_json.dump());

  }

  bool post_scan(state::scan scan)
  {
    using namespace flatbuffers;
    spdlog::get("cads")->info("Entering {}",__func__);
    auto [scanned_utc,db_name,uploaded,status] = scan;
    
    auto scanned_at = chrono::floor<chrono::seconds>(scanned_utc); // Default sends to much decimal precision for asp.net core
    
    if(!std::filesystem::exists(db_name))
    {
      spdlog::get("cads")->info("{}: {} doesn't exist. Scan not uploaded",__func__, db_name);
      return true;
    }

    auto profile_db_name = global_config["profile_db_name"].get<std::string>();
    auto endpoint_url = global_config["base_url"].get<std::string>();
    auto upload_profile = global_config["upload_profile"].get<bool>();

    if (endpoint_url == "null")
    {
      spdlog::get("cads")->info("{} endpoint_url set to null. Scan not uploaded",__func__);
      return true;
    }

    auto [params, err] = fetch_scan_gocator(db_name);
   
    if (err < 0)
    {
      spdlog::get("cads")->info("{} fetch_scan_gocator failed - {}",__func__, db_name);
      return true;
    }

    auto [rowid,ignored]  = fetch_scan_uploaded(db_name);
    auto fetch_profile = fetch_scan_coro(rowid,std::numeric_limits<int>::max(),db_name);

    FlatBufferBuilder builder(4096 * 128);
    std::vector<flatbuffers::Offset<CadsFlatbuffers::profile>> profiles_flat;

    auto ts = to_str(scanned_at);
    cpr::Url endpoint{mk_post_profile_url(ts)};


  
    auto YmaxN = zs_count(db_name);

    auto z_resolution = std::get<0>(params);
    auto z_offset = std::get<1>(params);
    auto y_step = global_belt_parameters.Length / (YmaxN);


    auto start = std::chrono::high_resolution_clock::now();
    int64_t size = 0;
    int64_t frame_idx = -1;
    double belt_z_max = 0;
    int final_idx = 0;

    while (true)
    {
      auto [co_terminate, cv] = fetch_profile.resume(0);
      auto [idx, zs] = cv;

      if (frame_idx < 0)
        frame_idx = (int64_t)idx;

      if (!co_terminate)
      {
        namespace sr = std::ranges;

        auto y = (idx - 1) * y_step;// Sqlite rowid starts a 1

        auto max_iter = max_element(zs.begin(), zs.end());

        if (max_iter != zs.end())
        {
          belt_z_max = max(belt_z_max, (double)*max_iter);
        }
 
        auto tmp_z = zs | sr::views::transform([=](float e) -> int16_t
                                                { return std::isnan(e) ? std::numeric_limits<int16_t>::lowest() : int16_t(((double)e - z_offset) / z_resolution); });
        std::vector<int16_t> short_z{tmp_z.begin(), tmp_z.end()};

        profiles_flat.push_back(CadsFlatbuffers::CreateprofileDirect(builder, y, 0, &short_z));

        if (profiles_flat.size() == communications_config.UploadRows)
        {
          size += send_flatbuffer_array(builder, z_resolution, z_offset, idx - communications_config.UploadRows, profiles_flat, endpoint, upload_profile);
          store_scan_uploaded(idx+1,db_name);
        }
        auto end = std::chrono::high_resolution_clock::now();
        auto duration = (end - start) / 1.0s;
        spdlog::get("cads")->info("Compressed Upload RATE(Kb/s):{} ", (zs.size()*sizeof(z_element)) / (1000 * duration));
      }
      else
      {
        if (profiles_flat.size() > 0)
        {
          size += send_flatbuffer_array(builder, z_resolution, z_offset, idx - profiles_flat.size(), profiles_flat, endpoint, upload_profile);
          store_scan_uploaded(idx+1,db_name);
        }
        final_idx = idx;
        break;
      }
    }


    bool failure = false;
    if (final_idx == YmaxN)
    {
      http_post_profile_properties2(ts, YmaxN, y_step);
    }else
    {
      failure = true;
      spdlog::get("cads")->error("{}: Number of profiles sent {} not matching expected {}", __func__, final_idx, YmaxN);
    }
    
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::seconds>(end - start).count() + 1; // Add one second to avoid divide by zero

    spdlog::get("cads")->info("ZMAX: {}, SIZE: {}, DUR:{}, RATE(Kb/s):{} ", belt_z_max, size, duration, size / (1000 * duration));
    spdlog::get("cads")->info("Leaving {}",__func__);
    return failure;

  }


  cads::coro<int, cads::profile, 1> post_profiles_coro(cads::meta meta)
  {
    using namespace flatbuffers;

    auto [endpoint_url, enable] = global_webapi.add_belt;

    FlatBufferBuilder builder(4096 * 128);
    std::vector<flatbuffers::Offset<CadsFlatbuffers::profile>> profiles_flat;

    cpr::Url endpoint{mk_post_profile_url(endpoint_url,meta.chrono)};

    int64_t size = 0;
    int64_t frame_idx = -1;
    double belt_z_max = 0;
    double belt_z_min = 0;

    while (true)
    {
      auto [p, co_terminate] = co_yield 0;
      if (!co_terminate)
      {
        namespace sr = std::ranges;

        auto max_iter = max_element(p.z.begin(), p.z.end());

        if (max_iter != p.z.end())
        {
          belt_z_max = max(belt_z_max, (double)*max_iter);
        }

        auto min_iter = min_element(p.z.begin(), p.z.end());

        if (min_iter != p.z.end())
        {
          belt_z_min = min(belt_z_min, (double)*min_iter);
        }

        auto tmp_z = p.z | sr::views::transform([=](float e) -> int16_t
                                                { return std::isnan(e) ? std::numeric_limits<int16_t>::lowest() : int16_t(((double)e - meta.z_off) / meta.z_res); });
        std::vector<int16_t> short_z{tmp_z.begin(), tmp_z.end()};

        profiles_flat.push_back(CadsFlatbuffers::CreateprofileDirect(builder, p.y, p.x_off, &short_z));

        if (profiles_flat.size() == 256)
        {
          size += send_flatbuffer_array(builder, meta.z_res, meta.z_off, frame_idx / 256, profiles_flat, endpoint, enable);
        }
      }
      else
      {
        break;
      }
    }

    if (profiles_flat.size() > 0)
    {
      size += send_flatbuffer_array(builder, meta.z_res, meta.z_off, frame_idx / 256, profiles_flat, endpoint, enable);
    }

    meta.z_max = belt_z_max;
    meta.z_min = belt_z_min;
    http_post_profile_properties(meta);

    co_return 0;
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
