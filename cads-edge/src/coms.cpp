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
#include <interpolation.h>

#pragma GCC diagnostic pop

#include <constants.h>
#include <utils.hpp>

using namespace std;

namespace
{

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
coro<remote_msg,bool> remote_control_coro()
  {

    using NC = std::unique_ptr<natsConnection, decltype(&natsConnection_Destroy)>;
    using NO = std::unique_ptr<natsOptions, decltype(&natsOptions_Destroy)>;
    using NS = std::unique_ptr<natsSubscription,decltype(&natsSubscription_Destroy)>;
 
    auto endpoint_url = communications_config.NatsUrl;

    auto nats_subject = fmt::format("{}Publish",constants_device.Serial);


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
          spdlog::get("cads")->error(R"({{ func = '{}' msg = '{}'}})", __func__, "Nats Connection closed");
          loop = false;
        }
        else if (s == NATS_OK)
        {
          auto cads_msg = CadsFlatbuffers::GetMsg(natsMsg_GetData(msg));
          auto msg_contents = cads_msg->contents_type();

          switch(msg_contents) {
            case CadsFlatbuffers::MsgContents_Start : {
              auto str = flatbuffers::GetString(cads_msg->contents_as_Start()->lua_code());
              auto rtn = Start{str};
              std::tie(terminate,terminate) = co_yield {rtn};
            }
            break;
            case CadsFlatbuffers::MsgContents_Stop : {
              auto rtn = Stop{};
              std::tie(terminate,terminate) = co_yield {rtn};
            }
            break;
            default:
            spdlog::get("cads")->error(R"({{ func = '{}' msg = 'Unkown id {}'}})", __func__, (long)msg_contents);
            break;
          }
        }else{}
        
        natsMsg_Destroy(msg);
      }
    }
    spdlog::get("cads")->error(R"({{ func = '{}' msg = '{}'}})", __func__, "Exiting coroutine");
  }

  void remote_control_thread(moodycamel::BlockingConcurrentQueue<remote_msg> &queue, bool &terminate)
  {

    while(!std::atomic_ref<bool>(terminate)) {
    auto remote_control = remote_control_coro();
   
      for(;!std::atomic_ref<bool>(terminate);) {
        auto [err,msg] = remote_control.resume(terminate);
        
        if(err) {
          break;
        }
        
        queue.enqueue(msg);
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

  void realtime_metrics_thread(moodycamel::BlockingConcurrentQueue<std::tuple<std::string, std::string, std::string>> &queue, HeartBeat beat, bool &terminate)
  {
    std::tuple<std::string, std::string, std::string> m;
    auto realtime_metrics = realtime_metrics_coro();

    if(beat.SendHeartBeat) {
      realtime_metrics.resume(std::make_tuple(beat.Subject,"",""));
    }

    auto trigger = std::chrono::system_clock::now() + beat.Period;
    
    for(;!std::atomic_ref<bool>(terminate);) {
      if(!queue.wait_dequeue_timed(m, std::chrono::milliseconds(500))) {
        auto now = std::chrono::system_clock::now();
        if(beat.SendHeartBeat && now > trigger) {
          trigger += beat.Period;
          realtime_metrics.resume(std::make_tuple(beat.Subject,"",""));
        }

        continue;
      }

      realtime_metrics.resume(m);
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

  std::string mk_post_profile_url(date::utc_clock::time_point time, std::string site, std::string conveyor)
  {
    
    // Default sends to much decimal precision for asp.net core
    auto ts = to_str(chrono::floor<chrono::seconds>(time)); 
    
    auto endpoint_url = global_config["base_url"].get<std::string>() + "/belt";

    if (endpoint_url == "null")
      return endpoint_url;

    return endpoint_url + '/' + site + '/' + conveyor + '/' + ts;
  }

  coro<long,std::tuple<std::vector<uint8_t>,std::size_t>,1> send_bytes_coro(long sent, std::string url,bool upload_profile) {

    
    auto [data,terminate_first] = co_yield 0;

    if(terminate_first) {
      co_return 0;
    }

    auto [data_first,data_tag] = data;

    if(data_first.size() == 0) co_return 0;
    
    auto sending = data_tag;

    std::vector<uint8_t> bufv_first = compress(std::move(data_first));    

    cpr::Url endpoint{url};
    auto response = upload_profile ? cpr::PostAsync(endpoint,cpr::Body{(char *)bufv_first.data(), bufv_first.size()}, cpr::Header{{"Content-Encoding", "br"}, {"Content-Type", "application/octet-stream"}}) : cpr::AsyncResponse();
    
    while(true) {
      auto [data,terminate] = co_yield sent;

      if(terminate) break;
      
      auto [buf,data_tag] = data;

      auto bufv = compress(buf); 

      if(upload_profile) {
        auto r = response.get();

        if (!(cpr::ErrorCode::OK == r.error.code && cpr::status::HTTP_OK == r.status_code))
        {
          spdlog::get("cads")->error(R"({{func ='{}', msg = 'Upload {} failed with http status code {}'}})", __func__, r.url.c_str(),r.status_code);
          break;
        }else{
          sent = sending;
        }
  
        if(bufv.size() > 0) {
          spdlog::get("cads")->debug(R"({{func ='{}', msg = 'Buffer Size:{} Compressed Size:{}'}})", __func__, buf.size(),bufv.size());
          sending = data_tag;
          response = cpr::PostAsync(endpoint,cpr::Body{(char *)bufv.data(), bufv.size()}, cpr::Header{{"Content-Encoding", "br"}, {"Content-Type", "application/octet-stream"}});
        }else {
          spdlog::get("cads")->error(R"({{func ='{}', msg = 'Buffer Size:{} Compressed Size:{}'}})", __func__, buf.size(),bufv.size());
          break;
        }
      }else {
        sent = sending;
        sending = data_tag;

        if(bufv.size() == 0) break;
      }
    }

    co_return sent;

  }
  
  
  void http_post_profile_properties_json(std::string json)
  {

    auto endpoint_url = global_config["base_url"].get<std::string>() + "/meta";
    auto upload_props = global_config["upload_profile"].get<bool>();

    if (endpoint_url == "null")
    {
      spdlog::get("cads")->info("base_url set to null. Config not uploaded");
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
        spdlog::get("cads")->debug(R"({{func ='{}', msg = '{}'}})",__func__,"Scan Posted");
        break;
      }
      else
      {
        spdlog::get("cads")->error("http_post_profile_properties_json failed with http status code {}, body: {}, endpoint: {}", r.status_code, json, endpoint.str());
      }
    }
  }

  void http_post_profile_properties2(date::utc_clock::time_point chrono, double YmaxN, double y_step, double z_min, double z_max, Conveyor conveyor, GocatorProperties gocator)
  {
    nlohmann::json params_json;

    params_json["site"] = conveyor.Site;
    params_json["conveyor"] = conveyor.Name;
    params_json["chrono"] = to_str(chrono::floor<chrono::seconds>(chrono));
    params_json["y_res"] = y_step;
    params_json["x_res"] = gocator.xResolution;
    params_json["z_res"] = gocator.zResolution;
    params_json["z_off"] = gocator.zOffset;
    params_json["z_min"] = z_min;
    params_json["z_max"] = z_max;
    params_json["Ymax"] = conveyor.Length;
    params_json["YmaxN"] = YmaxN;
    params_json["WidthN"] = conveyor.WidthN;
    params_json["Belt"] = conveyor.Belt;

    http_post_profile_properties_json(params_json.dump());
  }

  std::tuple<state::scan,bool> post_scan(state::scan scan)
  {
    using namespace flatbuffers;    
    
    if(scan.uploaded >= scan.cardinality) return {scan,false};
    
    auto db_name = scan.db_name;
    
    spdlog::get("cads")->info("Entering {}", __func__);

    if (!std::filesystem::exists(db_name))
    {
      spdlog::get("cads")->info("{}: {} doesn't exist. Scan not uploaded", __func__, db_name);
      return {scan,true};
    }
    
    auto [gocator, gocator_err] = fetch_scan_gocator(db_name);
    
    if (gocator_err < 0)
    {
      spdlog::get("cads")->info("{} fetch_scan_gocator failed - {}", __func__, db_name);
      return {scan,true};
    } 
    
    auto [conveyor, conveyor_err] = fetch_scan_conveyor(db_name);
    
    if (conveyor_err < 0)
    {
      spdlog::get("cads")->info("{} fetch_scan_conveyor failed - {}", __func__, db_name);
      return {scan,true};
    } 

    auto endpoint_url = mk_post_profile_url(scan.scanned_utc,conveyor.Site,conveyor.Name);
    auto upload_profile = global_config["upload_profile"].get<bool>();
    auto fetch_profile = fetch_scan_coro(scan.begin_index + 1 + scan.uploaded, scan.begin_index + scan.cardinality + 1, db_name);

    FlatBufferBuilder builder(4096 * 128);
    std::vector<flatbuffers::Offset<CadsFlatbuffers::profile>> profiles_flat;

    auto send_bytes = send_bytes_coro(0L,endpoint_url,upload_profile);

    auto YmaxN = scan.cardinality;

    auto z_resolution = gocator.zResolution;
    auto z_offset = gocator.zOffset;
    auto y_step = conveyor.Length / (YmaxN);

    double belt_z_max = std::numeric_limits<double>::lowest();
    double belt_z_min = std::numeric_limits<double>::max();
    long cnt = 0;

    while (true)
    {
      auto [co_terminate, cv] = fetch_profile.resume(0);
      auto [idx, zs_raw] = cv;

      if (!co_terminate)
      {
        namespace sr = std::ranges;
        auto zs = interpolate_to_widthn(zs_raw,conveyor.WidthN);
        idx -= scan.begin_index; cnt++;
        auto y = (idx - 1) * y_step; // Sqlite rowid starts a 1
        auto [pmin,pmax] = sr::minmax(zs | sr::views::filter([](float e) { return !std::isnan(e);}));

        belt_z_max = std::max(belt_z_max, (double)pmax);
        belt_z_min = std::min(belt_z_min, (double)pmin);

        auto tmp_z = zs | sr::views::transform([=](float e) -> int16_t
                                               { return std::isnan(e) ? std::numeric_limits<int16_t>::lowest() : int16_t(((double)e - z_offset) / z_resolution); });
        std::vector<int16_t> short_z{tmp_z.begin(), tmp_z.end()};

        profiles_flat.push_back(CadsFlatbuffers::CreateprofileDirect(builder, y, 0, &short_z));

        if (profiles_flat.size() == communications_config.UploadRows)
        {
          builder.Finish(CadsFlatbuffers::Createprofile_array(builder, z_resolution, z_offset, idx - communications_config.UploadRows, profiles_flat.size(), builder.CreateVector(profiles_flat)));

          auto [terminate,sent_rows] = send_bytes.resume({{builder.GetBufferPointer(),builder.GetBufferPointer()+builder.GetSize()},profiles_flat.size()});
          profiles_flat.clear(); builder.Clear();
          if(terminate) break;
          scan.uploaded += sent_rows;
          update_scan_state(scan);
          
        }
      }
      else
      {
        if (profiles_flat.size() > 0)
        {
          builder.Finish(CadsFlatbuffers::Createprofile_array(builder, z_resolution, z_offset, idx - profiles_flat.size(), profiles_flat.size(), builder.CreateVector(profiles_flat)));
          auto [terminate,sent_rows] = send_bytes.resume({{builder.GetBufferPointer(),builder.GetBufferPointer()+builder.GetSize()},profiles_flat.size()});
          profiles_flat.clear(); builder.Clear();
          if(terminate) break;
          scan.uploaded += sent_rows;
          update_scan_state(scan);

          //size += send_flatbuffer_array(builder, z_resolution, z_offset, idx - profiles_flat.size(), profiles_flat, endpoint, upload_profile);
          //store_scan_uploaded(idx + 1, db_name);
        }
        
        auto [terminate,sent_rows] = send_bytes.resume({std::vector<uint8_t>(),0});
        if(!terminate) {
          spdlog::get("cads")->info("{{func = {}, msg = 'Last resume not terminated'}}", __func__);
        }
          
        scan.uploaded += sent_rows;
        update_scan_state(scan);
        break;
      }
    }

    bool failure = scan.uploaded != YmaxN;
    if (!failure)
    {
      http_post_profile_properties2(scan.scanned_utc, YmaxN, y_step, belt_z_min, belt_z_max, conveyor, gocator);
    }

    return {scan,failure};
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
