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

#pragma GCC diagnostic pop


#include <constants.h>
#include <db.h>

using namespace std;

namespace {
  
  std::tuple<std::string,bool> http_post_json(std::string endpoint_url, std::string json, int retry = 16, int retry_wait = 8)
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
        return {r.text,false};
      }
      else
      {
        if(retry == 0) {
          spdlog::get("upload")->error("http_post_json failed with http status code {}, body: {}, endpoint: {}", r.status_code,json,endpoint.str());
        }else{
          std::this_thread::sleep_for(std::chrono::seconds(retry_wait));
        }
      }
    }

    return {"",true};
  }
}

namespace cads
{
  moodycamel::BlockingConcurrentQueue<std::tuple<std::string,std::string>> nats_queue;

  void remote_control_thread(moodycamel::BlockingConcurrentQueue<int> &nats_queue, bool& terminate) {
    
    auto endpoint_url = global_config["nats_url"].get<std::string>();
    
    natsConnection  *conn  = nullptr;
    natsOptions *opts = nullptr;
    natsSubscription *sub  = nullptr;
    

    for(;!terminate;) {
      auto status = natsOptions_Create(&opts);
      if(status != NATS_OK) goto drop_msg;
      
      natsOptions_SetAllowReconnect(opts,true);
      natsOptions_SetURL(opts,endpoint_url.c_str());

      status = natsConnection_Connect(&conn, opts);
      if(status != NATS_OK) goto cleanup_opts;

      status = natsConnection_SubscribeSync(&sub, conn, "originReset");
      if(status != NATS_OK) goto cleanup_opts;

      for(auto loop = true;loop && !terminate;) {
        
        natsMsg *msg  = nullptr;
        auto s = natsSubscription_NextMsg(&msg, sub, 500);

        if(s == NATS_TIMEOUT) {

        }else if(s == NATS_CONNECTION_CLOSED) {
          loop = false; 
        }
        else if(s == NATS_OK) {
          std::string sub(natsMsg_GetSubject(msg));
          std::string msg_string(natsMsg_GetData(msg),natsMsg_GetDataLength(msg));
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
        if(nats_queue.wait_dequeue_timed(dropped_msg,std::chrono::seconds(1))) 
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


  std::tuple<int,bool> remote_addconveyor(Conveyor params) {
    auto [url,enable] = global_webapi.add_conveyor;
    
    if(enable) {
      auto [text,err] = http_post_json(url,params);
      auto json = nlohmann::json::parse(text,nullptr,false);
      
      if(json.is_number_integer()){
        auto conveyor_id = json.get<int>();
        return {conveyor_id,err};
      }else{
        return {0,true};
      }
    }else{
      return {0,true};
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

    while (upload_profile)
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
    params_json["Site"] = global_conveyor_parameters.Site;
    params_json["Conveyor"] = global_conveyor_parameters.Name;
    params_json["Time"] = ts;
    params_json["YArea"] = y_area;
    params_json["Value"] = value;
    
    nats_queue.enqueue({"/realtime",params_json.dump()});

  }

  void publish_meta_realtime(std::string Id, double value, bool valid)
  {
    
    if(isnan(value) || isinf(value)) return;

    nlohmann::json params_json;
    params_json["Site"] = global_conveyor_parameters.Site;
    params_json["Conveyor"] = global_conveyor_parameters.Name;
    params_json["Id"] = Id;
    params_json["Value"] = value;
    params_json["Valid"] = valid; 
    
    nats_queue.enqueue({"/realtimemeta",params_json.dump()});
 
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
        spdlog::get("upload")->error("http_post_profile_properties_json failed with http status code {}, body: {}, endpoint: {}", r.status_code,json,endpoint.str());
      }
    }
  }

  void http_post_profile_properties(int revid, int last_idx, std::string chrono, double belt_length, double y_step)
  {
    nlohmann::json params_json;
    auto db_name = global_config["profile_db_name"].get<std::string>();
    auto [params, err] = fetch_profile_parameters(db_name);
    auto [Ymin,Ymax,YmaxN,WidthN,err2] = fetch_belt_dimensions(revid,last_idx,db_name);

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

    if(err == 0 && err2 == 0) {
      http_post_profile_properties_json(params_json.dump());
    }
  }

  std::tuple<date::utc_clock::time_point,bool> http_post_whole_belt(int revid, int last_idx, int first_idx)
  {
    using namespace flatbuffers;

    auto now = chrono::floor<chrono::seconds>(date::utc_clock::now()); // Default sends to much decimal precision for asp.net core
    auto db_name = global_config["profile_db_name"].get<std::string>();
    auto endpoint_url = global_config["base_url"].get<std::string>();
    auto y_max_length = global_config["y_max_length"].get<double>();
    auto belt_length = global_config["y_max_upload"].get<double>();
    auto upload_profile = global_config["upload_profile"].get<bool>();


    if (endpoint_url == "null")
    {
      spdlog::get("upload")->info("http_post_whole_belt set to null. Belt not uploaded");
      return {now,true};
    }

    auto fetch_profile = fetch_belt_coro(revid, last_idx);

    FlatBufferBuilder builder(4096 * 128);
    std::vector<flatbuffers::Offset<CadsFlatbuffers::profile>> profiles_flat;

    auto ts = date::format("%FT%TZ", now);
    cpr::Url endpoint{mk_post_profile_url(ts)};

    auto [params, err] = fetch_profile_parameters(db_name);
    auto [Ymin,Ymax,YmaxN,WidthN,err2] = fetch_belt_dimensions(revid,last_idx,db_name);
    
    if (err != 0)
    {
      spdlog::get("upload")->error("Unable to fetch profile parameters from DB. Revid: {}", revid);
      return {now,true};
    }

    auto z_resolution = params.z_res;
    auto z_offset =  params.z_off;
    auto y_step = belt_length / (YmaxN);

    if(std::floor(y_max_length * 0.75 / params.y_res) > last_idx) {
      spdlog::get("upload")->error("Belt less than 0.75 of max belt length. Length of {}, revid: {}", belt_length, revid);
      return {now,true};
    }

    auto cnt_width_n = count_with_width_n(db_name, revid, WidthN);

    if(cnt_width_n < 0 && cnt_width_n != YmaxN) {
      spdlog::get("upload")->error("Profiles of belt not same number of samples. revid: {}", revid);
      return {now,true};
    }

    if((last_idx-1) != (int)YmaxN) {
      spdlog::get("upload")->error("Database doesn't contain the number of profiles requested. revid: {}, requested: {}, retrieved:{}", revid, last_idx-1, YmaxN);
      return {now,true};
    }

    auto start = std::chrono::high_resolution_clock::now();
    int64_t size = 0;
    int64_t frame_idx = -1;
    double belt_z_max = 0; 
    double y_adjustment = 0.0;
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

        p.y = y_adjustment;
        y_adjustment += y_step;

        auto max_iter = max_element(p.z.begin(), p.z.end());
        
        if(max_iter != p.z.end()) {
          belt_z_max = max(belt_z_max, (double)*max_iter);
        }

        auto tmp_z = p.z | sr::views::transform([=](float e) -> int16_t
                                                { return std::isnan(e) ? std::numeric_limits<int16_t>::lowest() : int16_t(((double)e - z_offset) / z_resolution); });
        std::vector<int16_t> short_z{tmp_z.begin(), tmp_z.end()};

        profiles_flat.push_back(CadsFlatbuffers::CreateprofileDirect(builder, p.y, p.x_off, &short_z));

        if (profiles_flat.size() == 256)
        {
          size += send_flatbuffer_array(builder, z_resolution, z_offset, frame_idx, profiles_flat, endpoint, upload_profile);
          frame_idx = -1;
        }
      }
      else
      {
        if (profiles_flat.size() > 0)
        {
          size += send_flatbuffer_array(builder, z_resolution, z_offset, frame_idx, profiles_flat, endpoint, upload_profile);
        }
        final_idx = idx;
        break;
      }
    }

    y_adjustment -= y_step;

    if(std::abs((y_adjustment / belt_length) - 1) > 0.001 ) {
      spdlog::get("upload")->error("Uploaded belt length validation failed. revid: {}, requested: {}, retrieved:{}", revid, belt_length, y_adjustment);
      return {now,true};
    }

    bool failure = false;
    if(final_idx == last_idx-1) {
      http_post_profile_properties(revid,last_idx,ts,y_adjustment,y_step);
    }else{
      failure = true;
      spdlog::get("upload")->error("Number of profiles sent {} not matching idx of {}. Revid id: {}", final_idx+1,last_idx, revid);
    }

    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::seconds>(end - start).count() + 1; // Add one second to avoid divide by zero

    spdlog::get("upload")->info("ZMAX: {}, SIZE: {}, DUR:{}, RATE(Kb/s):{} ", belt_z_max, size, duration, size / (1000 * duration));
    spdlog::get("upload")->info("Leaving http_post_thread_bulk");
    auto program_state_db_name = global_config["state_db_name"].get<std::string>();
    auto next_upload_date = fetch_daily_upload(program_state_db_name);
    next_upload_date += std::chrono::days(1);
    store_daily_upload(next_upload_date,program_state_db_name);
    return {now,failure};
  }



std::tuple<date::utc_clock::time_point,bool> http_post_belt(int revid, int last_idx, int first_idx, std::string db_name)
  {
    using namespace flatbuffers;

    auto now = chrono::floor<chrono::seconds>(date::utc_clock::now()); // Default sends to much decimal precision for asp.net core
    auto endpoint_url = global_config["base_url"].get<std::string>();
    auto y_max_length = global_config["y_max_length"].get<double>();
    auto belt_length = global_config["y_max_upload"].get<double>();
    auto upload_profile = global_config["upload_profile"].get<bool>();


    if (endpoint_url == "null")
    {
      spdlog::get("upload")->info("http_post_whole_belt set to null. Belt not uploaded");
      return {now,true};
    }

    auto fetch_profile = fetch_belt_coro(revid, last_idx,first_idx,256,db_name);

    FlatBufferBuilder builder(4096 * 128);
    std::vector<flatbuffers::Offset<CadsFlatbuffers::profile>> profiles_flat;

    auto ts = date::format("%FT%TZ", now);
    cpr::Url endpoint{mk_post_profile_url(ts)};

    auto [params, err] = fetch_profile_parameters(db_name);
    auto [Ymin,Ymax,YmaxN,WidthN,err2] = fetch_belt_dimensions(revid,last_idx,db_name);
    
    if (err != 0)
    {
      spdlog::get("upload")->error("Unable to fetch profile parameters from DB. Revid: {}", revid);
      return {now,true};
    }

    auto z_resolution = params.z_res;
    auto z_offset =  params.z_off;
    auto y_step = belt_length / (YmaxN);

    if(std::floor(y_max_length * 0.75 / params.y_res) > last_idx) {
      spdlog::get("upload")->error("Belt less than 0.75 of max belt length. Length of {}, revid: {}", belt_length, revid);
      return {now,true};
    }

    auto cnt_width_n = count_with_width_n(db_name, revid, WidthN);

    if(cnt_width_n < 0 && cnt_width_n != YmaxN) {
      spdlog::get("upload")->error("Profiles of belt not same number of samples. revid: {}", revid);
      return {now,true};
    }

    if((last_idx-1) != (int)YmaxN) {
      spdlog::get("upload")->error("Database doesn't contain the number of profiles requested. revid: {}, requested: {}, retrieved:{}", revid, last_idx-1, YmaxN);
      return {now,true};
    }

    int64_t size = 0;
    int64_t frame_idx = -1;
    double belt_z_max = 0; 
    double y_adjustment = 0.0;
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

        p.y = y_adjustment;
        y_adjustment += y_step;

        auto max_iter = max_element(p.z.begin(), p.z.end());
        
        if(max_iter != p.z.end()) {
          belt_z_max = max(belt_z_max, (double)*max_iter);
        }

        auto tmp_z = p.z | sr::views::transform([=](float e) -> int16_t
                                                { return std::isnan(e) ? std::numeric_limits<int16_t>::lowest() : int16_t(((double)e - z_offset) / z_resolution); });
        std::vector<int16_t> short_z{tmp_z.begin(), tmp_z.end()};

        profiles_flat.push_back(CadsFlatbuffers::CreateprofileDirect(builder, p.y, p.x_off, &short_z));

        if (profiles_flat.size() == 256)
        {
          size += send_flatbuffer_array(builder, z_resolution, z_offset, frame_idx, profiles_flat, endpoint, upload_profile);
          frame_idx = -1;
        }
      }
      else
      {
        if (profiles_flat.size() > 0)
        {
          size += send_flatbuffer_array(builder, z_resolution, z_offset, frame_idx, profiles_flat, endpoint, upload_profile);
        }
        final_idx = idx;
        break;
      }
    }

    y_adjustment -= y_step;

    if(std::abs((y_adjustment / belt_length) - 1) > 0.001 ) {
      spdlog::get("upload")->error("Uploaded belt length validation failed. revid: {}, requested: {}, retrieved:{}", revid, belt_length, y_adjustment);
      return {now,true};
    }

    bool failure = false;
    if(final_idx == last_idx-1) {
      http_post_profile_properties(revid,last_idx,ts,y_adjustment,y_step);
    }else{
      failure = true;
      spdlog::get("upload")->error("Number of profiles sent {} not matching idx of {}. Revid id: {}", final_idx+1,last_idx, revid);
    }

    return {now,failure};
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
