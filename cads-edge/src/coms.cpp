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
#include <cads_msg_generated.h>
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

  std::vector<uint8_t> compress(uint8_t *input, uint32_t size)
  {

    if(size < 1) return {};

    std::vector<uint8_t> output(size); // Assume output vector < size

    size_t input_size = size;
    size_t output_size = output.size();

    BrotliEncoderCompress(8, BROTLI_DEFAULT_WINDOW, BROTLI_DEFAULT_MODE, input_size, input, &output_size, output.data());
    output.resize(output_size);
    return output;
  }

  std::vector<uint8_t> compress(std::vector<uint8_t> input)
  {
    return ::compress(input.data(),input.size());
  }
}

namespace cads
{

  void remote_control_thread(bool &terminate, moodycamel::BlockingConcurrentQueue<remote_msg> &queue)
  {

    using NC = std::unique_ptr<natsConnection, decltype(&natsConnection_Destroy)>;
    using NO = std::unique_ptr<natsOptions, decltype(&natsOptions_Destroy)>;
    using NS = std::unique_ptr<natsSubscription,decltype(&natsSubscription_Destroy)>;
 
    auto endpoint_url = communications_config.NatsUrl;

    auto nats_subject = fmt::format("{}",constants_device.Serial);


    for (; !terminate;)
    {
      natsOptions *opts_raw = nullptr;
      auto status = natsOptions_Create(&opts_raw);
      if (status != NATS_OK){ 
        spdlog::get("cads")->error("{}:natsOptions_Create->{}", __func__, (int)status);
        continue;
      }

      NO opts{opts_raw,natsOptions_Destroy};

      if((status = natsOptions_SetAllowReconnect(opts.get(), false)) != NATS_OK){ 
        spdlog::get("cads")->error("{}:natsOptions_SetAllowReconnect->{}", __func__, (int)status);
        continue;
      }

      natsOptions_SetURL(opts.get(), endpoint_url.c_str());

      natsConnection *conn_raw = nullptr;
      status = natsConnection_Connect(&conn_raw, opts.get());
      if (status != NATS_OK) continue;

      NC conn{conn_raw,natsConnection_Destroy};

      natsSubscription *sub_raw = nullptr;
      status = natsConnection_SubscribeSync(&sub_raw, conn.get(), nats_subject.c_str());
      if (status != NATS_OK){ 
        spdlog::get("cads")->error("{}:natsConnection_SubscribeSync->{}", __func__, (int)status);
        continue;
      }

      NS sub{sub_raw,natsSubscription_Destroy};

      for (auto loop = true; loop && !terminate;)
      {

        natsMsg *msg = nullptr;
        auto s = natsSubscription_NextMsg(&msg, sub.get(), 500);

        if (s == NATS_TIMEOUT)
        {
        }
        else if (s == NATS_CONNECTION_CLOSED)
        {
          loop = false;
        }
        else if (s == NATS_OK)
        {
          auto cads_msg = CadsFlatbuffers::GetMsg(natsMsg_GetData(msg));
          auto msg_contents = cads_msg->contents_type();

          switch(msg_contents) {
            case CadsFlatbuffers::MsgContents_Start : {
              auto str = flatbuffers::GetString(cads_msg->contents_as_Start()->lua_code());
              queue.enqueue({Start{str}});
            }
            break;
            default:
            break;
          }
         }
        
        natsMsg_Destroy(msg);
      }
    }
  }

  coro<remote_msg,bool> remote_control_coro()
  {

    using NC = std::unique_ptr<natsConnection, decltype(&natsConnection_Destroy)>;
    using NO = std::unique_ptr<natsOptions, decltype(&natsOptions_Destroy)>;
    using NS = std::unique_ptr<natsSubscription,decltype(&natsSubscription_Destroy)>;
 
    auto endpoint_url = communications_config.NatsUrl;

    auto nats_subject = fmt::format("{}",constants_device.Serial);


    for (bool terminate = false; !terminate;)
    {
      natsOptions *opts_raw = nullptr;
      auto status = natsOptions_Create(&opts_raw);
      if (status != NATS_OK){ 
        spdlog::get("cads")->error("{}:natsOptions_Create->{}", __func__, (int)status);
        continue;
      }

      NO opts{opts_raw,natsOptions_Destroy};

      if((status = natsOptions_SetAllowReconnect(opts.get(), false)) != NATS_OK){ 
        spdlog::get("cads")->error("{}:natsOptions_SetAllowReconnect->{}", __func__, (int)status);
        continue;
      }

      natsOptions_SetURL(opts.get(), endpoint_url.c_str());

      natsConnection *conn_raw = nullptr;
      status = natsConnection_Connect(&conn_raw, opts.get());
      if (status != NATS_OK) continue;

      NC conn{conn_raw,natsConnection_Destroy};

      natsSubscription *sub_raw = nullptr;
      status = natsConnection_SubscribeSync(&sub_raw, conn.get(), nats_subject.c_str());
      if (status != NATS_OK){ 
        spdlog::get("cads")->error("{}:natsConnection_SubscribeSync->{}", __func__, (int)status);
        continue;
      }

      NS sub{sub_raw,natsSubscription_Destroy};

      for (auto loop = true; loop && !terminate;)
      {

        natsMsg *msg = nullptr;
        auto s = natsSubscription_NextMsg(&msg, sub.get(), 500);

        if (s == NATS_TIMEOUT)
        {
          std::tie(terminate,terminate) = co_yield {Timeout{}};
        }
        else if (s == NATS_CONNECTION_CLOSED)
        {
          loop = false;
        }
        else if (s == NATS_OK)
        {
          auto cads_msg = CadsFlatbuffers::GetMsg(natsMsg_GetData(msg));
          auto msg_contents = cads_msg->contents_type();

          switch(msg_contents) {
            case CadsFlatbuffers::MsgContents_Start : {
              auto str = flatbuffers::GetString(cads_msg->contents_as_Start()->lua_code());
              std::tie(terminate,terminate) = co_yield {Start{str}};
            }
            break;
            case CadsFlatbuffers::MsgContents_Stop : {
              std::tie(terminate,terminate) = co_yield {Stop{}};
            }
            default:
            break;
          }
         }
        
        natsMsg_Destroy(msg);
      }
    }
  }

  cads::coro<int, std::tuple<std::string, std::string, std::string>, 1> realtime_metrics_coro()
  {

    using NC = std::unique_ptr<natsConnection, decltype(&natsConnection_Destroy)>;
    using NO = std::unique_ptr<natsOptions, decltype(&natsOptions_Destroy)>;
    using NM = std::unique_ptr<natsMsg, decltype(&natsMsg_Destroy)>;
    
    auto endpoint_url = communications_config.NatsUrl;
    bool terminate = false;

    for (std::tuple<std::string, std::string, std::string> msg; !terminate;  std::tie(msg, terminate) = co_yield 0)
    {
      natsOptions *opts_raw = nullptr;
      auto status = natsOptions_Create(&opts_raw);
      if (status != NATS_OK){ 
        spdlog::get("cads")->error("{}:natsOptions_Create->{}", __func__, (int)status);
        continue;
      }

      NO opts{opts_raw,natsOptions_Destroy};

      if((status = natsOptions_SetSendAsap(opts.get(), true)) != NATS_OK) {
        spdlog::get("cads")->error("{}:natsOptions_SetSendAsap->{}", __func__, (int)status);
        continue;
      }
      if((status = natsOptions_SetAllowReconnect(opts.get(), false)) != NATS_OK){ 
        spdlog::get("cads")->error("{}:natsOptions_SetAllowReconnect->{}", __func__, (int)status);
        continue;
      }
      if((status = natsOptions_SetURL(opts.get(), endpoint_url.c_str())) != NATS_OK) { 
        spdlog::get("cads")->error("{}:natsOptions_SetURL->{}", __func__, (int)status);
        continue;
      }
      
      natsConnection *conn_raw = nullptr;
      status = natsConnection_Connect(&conn_raw, opts.get());
      if (status != NATS_OK) continue;

      NC conn{conn_raw,natsConnection_Destroy};

      for (auto loop = true; loop && !terminate;)
      {

        std::tie(msg, terminate) = co_yield 0;

        if (terminate) continue;

        auto [sub, head, data] = msg;

        spdlog::get("cads")->debug("{}:natsConnection_PublishString {},{}", __func__, sub.c_str(), data.c_str());

        natsMsg *nats_msg_raw = nullptr;
 
        if ((status = natsMsg_Create(&nats_msg_raw, sub.c_str(), nullptr, data.c_str(), data.size())) != NATS_OK)
        {
          spdlog::get("cads")->error("{}:natsMsg_Create->{}", __func__, (int)status);
          continue;
        }

        NM nats_msg{nats_msg_raw,natsMsg_Destroy};

        if ((status = natsMsgHeader_Add(nats_msg.get(), "category", head.c_str())) != NATS_OK)
        {
          spdlog::get("cads")->error("{}:natsMsgHeader_Add->{}", __func__, (int)status);
          continue;
        }

        if ((status = natsConnection_PublishMsg(conn.get(), nats_msg.get())) != NATS_OK)
        {
          spdlog::get("cads")->error("{}:natsConnection_PublishString->{}", __func__, (int)status);
          loop = false;
        }
      }
    }
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

  
  coro<long,std::tuple<std::vector<uint8_t>,long>,1> send_flatbuffer_coro(std::string url,bool upload_profile) {

    
    auto [data,terminate_first] = co_yield 0;

    if(terminate_first) {
      co_return 0;
    }

    auto [data_first,data_id] = data;

    if(data_first.size() == 0) co_return 0;
    
    auto sending = data_id;
    auto sent = 0L;

    std::vector<uint8_t> bufv_first = compress(std::move(data_first));    

    cpr::Url endpoint{url};
    auto response = cpr::PostAsync(endpoint,cpr::Body{(char *)bufv_first.data(), bufv_first.size()}, cpr::Header{{"Content-Encoding", "br"}, {"Content-Type", "application/octet-stream"}});
    
    while(true) {
      auto [data,terminate] = co_yield sent;

      if(terminate) break;
      
      auto [buf,data_id] = data;

      auto bufv = compress(buf); 

      auto r = response.get();

      if (!(cpr::ErrorCode::OK == r.error.code && cpr::status::HTTP_OK == r.status_code))
      {
        spdlog::get("upload")->error("Upload failed with http status code {}", r.status_code);
        break;
      }else{
        sent = sending;
      }

      if(bufv.size()) {
        sending = data_id;
        response = cpr::PostAsync(endpoint,cpr::Body{(char *)bufv.data(), bufv.size()}, cpr::Header{{"Content-Encoding", "br"}, {"Content-Type", "application/octet-stream"}});
      }
    }

    co_return sent;

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

    std::vector<uint8_t> bufv = compress(buf, size);

    while (upload_profile)
    {
      cpr::Response r = cpr::Post(endpoint,
                                  cpr::Body{(char *)bufv.data(), bufv.size()},
                                  cpr::Header{{"Content-Encoding", "br"}, {"Content-Type", "application/octet-stream"}});

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

  void http_post_profile_properties2(std::string chrono, double YmaxN, double y_step, double z_max)
  {
    nlohmann::json params_json;
    auto db_name = global_config["profile_db_name"].get<std::string>();
    auto [params, err] = fetch_profile_parameters(db_name);
    auto [belt_id, err2] = fetch_belt_id();

    params_json["site"] = global_conveyor_parameters.Site;
    params_json["conveyor"] = global_conveyor_parameters.Name;
    params_json["chrono"] = chrono;
    params_json["y_res"] = y_step;
    params_json["x_res"] = params.xResolution;
    params_json["z_res"] = params.zResolution;
    params_json["z_off"] = params.zOffset;
    params_json["z_max"] = z_max;
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
    spdlog::get("cads")->info("Entering {}", __func__);
    auto [scanned_utc, db_name, uploaded, status] = scan;

    auto scanned_at = chrono::floor<chrono::seconds>(scanned_utc); // Default sends to much decimal precision for asp.net core

    if (!std::filesystem::exists(db_name))
    {
      spdlog::get("cads")->info("{}: {} doesn't exist. Scan not uploaded", __func__, db_name);
      return true;
    }

    auto profile_db_name = global_config["profile_db_name"].get<std::string>();
    auto endpoint_url = global_config["base_url"].get<std::string>();
    auto upload_profile = global_config["upload_profile"].get<bool>();

    if (endpoint_url == "null")
    {
      spdlog::get("cads")->info("{} endpoint_url set to null. Scan not uploaded", __func__);
      return true;
    }

    auto [params, err] = fetch_scan_gocator(db_name);

    if (err < 0)
    {
      spdlog::get("cads")->info("{} fetch_scan_gocator failed - {}", __func__, db_name);
      return true;
    }

    auto [rowid, ignored] = fetch_scan_uploaded(db_name);
    auto fetch_profile = fetch_scan_coro(rowid, std::numeric_limits<int>::max(), db_name);

    FlatBufferBuilder builder(4096 * 128);
    std::vector<flatbuffers::Offset<CadsFlatbuffers::profile>> profiles_flat;

    auto ts = to_str(scanned_at);
    cpr::Url endpoint{mk_post_profile_url(ts)};

    auto YmaxN = zs_count(db_name);

    auto z_resolution = params.zResolution;
    auto z_offset = params.zOffset;
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

        auto y = (idx - 1) * y_step; // Sqlite rowid starts a 1

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
          store_scan_uploaded(idx + 1, db_name);
        }
        // auto end = std::chrono::high_resolution_clock::now();
        // auto duration = (end - start) / 1.0s;
        // spdlog::get("cads")->info("Upload RATE(Kb/s):{} ", size / (1000 * duration));
      }
      else
      {
        if (profiles_flat.size() > 0)
        {
          size += send_flatbuffer_array(builder, z_resolution, z_offset, idx - profiles_flat.size(), profiles_flat, endpoint, upload_profile);
          store_scan_uploaded(idx + 1, db_name);
        }
        final_idx = idx;
        break;
      }
    }

    bool failure = false;
    if (final_idx == YmaxN)
    {
      http_post_profile_properties2(ts, YmaxN, y_step,belt_z_max);
    }
    else
    {
      failure = true;
      spdlog::get("cads")->error("{}: Number of profiles sent {} not matching expected {}", __func__, final_idx, YmaxN);
    }

    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::seconds>(end - start).count() + 1; // Add one second to avoid divide by zero

    spdlog::get("cads")->info("ZMAX: {}, SIZE: {}, DUR:{}, RATE(Kb/s):{} ", belt_z_max, size, duration, size / (1000 * duration));
    spdlog::get("cads")->info("Leaving {}", __func__);
    return failure;
  }

#if 0
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
#endif
}
