#include <chrono>
#include <ranges>
#include <algorithm>
#include <tuple>
#include <expected>

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
#include <sampling.h>

#pragma GCC diagnostic pop

#include <utils.hpp>

using namespace std;

namespace
{
  std::vector<int16_t> z_as_int16(cads::z_type z,double z_resolution, double z_offset) {
    namespace sr = std::ranges;
  
    auto tmp_z = z | sr::views::transform([=](float e) -> int16_t
                                            { return std::isnan(e) ? std::numeric_limits<int16_t>::lowest() : int16_t(((double)e - z_offset) / z_resolution); });
    return {tmp_z.begin(), tmp_z.end()};  
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

  void send_bytes_simple(std::vector<uint8_t> data,std::string url, bool upload)
  {
    auto bufv = ::compress(std::move(data));
    if(upload) {
      cpr::Url endpoint{url};
      cpr::Post(endpoint,cpr::Body{(char *)bufv.data(), bufv.size()}, cpr::Header{{"Content-Encoding", "br"}, {"Content-Type", "application/octet-stream"}});
    }
  }

  void post_register_scan_table(std::string belt_serial, std::string url, bool upload)
  {
    using namespace flatbuffers;
    FlatBufferBuilder builder;
    auto beltserial = builder.CreateString(belt_serial);
    CadsFlatbuffers::register_scanBuilder buf(builder);
    buf.add_belt_serial(beltserial);
    auto send = buf.Finish();

    CadsFlatbuffers::scanBuilder post(builder);
    post.add_contents_type(CadsFlatbuffers::scan_tables_register_scan);
    post.add_contents(send.Union());
    auto bytes = post.Finish();
    
    builder.Finish(bytes);

    send_bytes_simple({builder.GetBufferPointer(),builder.GetBufferPointer()+builder.GetSize()},url,upload);
  }

  void post_install_table(cads::Conveyor c, cads::Belt b, std::string url, bool upload)
  {
    using namespace flatbuffers;
    
    FlatBufferBuilder builder;
    auto site = builder.CreateString(c.Site);
    auto name = builder.CreateString(c.Name);
    auto timezone = builder.CreateString(c.Timezone);

    CadsFlatbuffers::ConveyorBuilder buf_conveyor(builder);
    buf_conveyor.add_site(site);
    buf_conveyor.add_name(name);
    buf_conveyor.add_timezone(timezone);
    buf_conveyor.add_pulley_circumference(c.PulleyCircumference);
    buf_conveyor.add_typical_speed(c.TypicalSpeed);
    auto send_conveyor = buf_conveyor.Finish();

    auto serial = builder.CreateString(b.Serial);
    CadsFlatbuffers::BeltBuilder buf_belt(builder);
    buf_belt.add_serial(serial);
    buf_belt.add_pulley_cover(b.PulleyCover);
    buf_belt.add_cord_diameter(b.CordDiameter);
    buf_belt.add_top_cover(b.TopCover);
    buf_belt.add_width(b.Width);
    buf_belt.add_width_n(b.WidthN);
    auto send_belt = buf_belt.Finish();

    CadsFlatbuffers::InstallBuilder buf_install(builder);
    buf_install.add_belt(send_belt);
    buf_install.add_conveyor(send_conveyor);
    auto send_install = buf_install.Finish();

    CadsFlatbuffers::scanBuilder post(builder);
    post.add_contents_type(CadsFlatbuffers::scan_tables_Install);
    post.add_contents(send_install.Union());
    auto bytes = post.Finish();

    builder.Finish(bytes);

    send_bytes_simple({builder.GetBufferPointer(),builder.GetBufferPointer()+builder.GetSize()},url,upload);
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

  void remote_control_thread(moodycamel::BlockingConcurrentQueue<remote_msg> &queue, std::atomic<bool> &terminate)
  {
    spdlog::get("cads")->debug(R"({{ func = '{}' msg = '{}'}})", __func__, "Entering Thread");

    while(!terminate) {
    auto remote_control = remote_control_coro();
   
      for(;!terminate;) {
        auto [err,msg] = remote_control.resume(terminate);
        
        if(err) {
          break;
        }
        
        queue.enqueue(msg);
      }
    }

    spdlog::get("cads")->debug(R"({{ func = '{}' msg = '{}'}})", __func__, "Exiting Thread");
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

        spdlog::get("cads")->trace("{}:natsConnection_PublishString {},{}", __func__, sub.c_str(), data.c_str());

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

  void realtime_metrics_thread(moodycamel::BlockingConcurrentQueue<std::tuple<std::string, std::string, std::string>> &queue, HeartBeat beat, std::atomic<bool> &terminate)
  {
    spdlog::get("cads")->debug(R"({{func = '{}', msg = '{}'}})", __func__,"Entering Thread");
    std::tuple<std::string, std::string, std::string> m;
    auto realtime_metrics = realtime_metrics_coro();

    if(beat.SendHeartBeat) {
      realtime_metrics.resume(std::make_tuple(beat.Subject,"",""));
    }

    auto trigger = std::chrono::system_clock::now() + beat.Period;
    
    for(;!terminate;) {
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

    spdlog::get("cads")->debug(R"({{func = '{}', msg = '{}'}})", __func__,"Exiting Thread");
  }

  std::string mk_post_profile_url(date::utc_clock::time_point time, std::string site, std::string conveyor, std::string endpoint_url)
  {
    
    // Default sends to much decimal precision for asp.net core
    auto ts = to_str(chrono::floor<chrono::seconds>(time)); 

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
  
  cads::state::scan post_profiles_table(cads::state::scan scan, std::string url, double y_step, size_t width_n, bool upload)
  {
    using namespace flatbuffers; 

    auto fetch_profile = fetch_scan_coro(scan.begin_index + 1 + scan.uploaded, scan.begin_index + scan.cardinality + 1, scan.db_name);

    FlatBufferBuilder builder(4096 * 128);
    std::vector<flatbuffers::Offset<CadsFlatbuffers::profile>> profiles_flat;

    auto send_bytes = send_bytes_coro(0L,url,upload);

    double belt_z_max = std::numeric_limits<double>::lowest();
    double belt_z_min = std::numeric_limits<double>::max();
    long cnt = 0;

    while (true)
    {
      auto [co_terminate, cv] = fetch_profile.resume(0);
      auto [idx, profile] = cv;

      if (!co_terminate)
      {
        namespace sr = std::ranges;
        auto zs = scan.remote_reg ? interpolate_to_widthn(profile.z,width_n) : profile.z;
        idx -= scan.begin_index; cnt++;
        auto y = (idx - 1) * y_step; // Sqlite rowid starts a 1
        auto [pmin,pmax] = sr::minmax(zs | sr::views::filter([](float e) { return !std::isnan(e);}));

        belt_z_max = std::max(belt_z_max, (double)pmax);
        belt_z_min = std::min(belt_z_min, (double)pmin);

        profiles_flat.push_back(CadsFlatbuffers::CreateprofileDirect(builder, y, profile.x_off, &zs));

        if (profiles_flat.size() == communications_config.UploadRows)
        {
          auto send = CadsFlatbuffers::Createprofile_array(builder, idx - communications_config.UploadRows, profiles_flat.size(), builder.CreateVector(profiles_flat));
          CadsFlatbuffers::scanBuilder post(builder);
          post.add_contents_type(CadsFlatbuffers::scan_tables_profile_array);
          post.add_contents(send.Union());
          auto bytes = post.Finish();

          builder.Finish(bytes);

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
          
          auto send = CadsFlatbuffers::Createprofile_array(builder, idx - profiles_flat.size(), profiles_flat.size(), builder.CreateVector(profiles_flat));
          CadsFlatbuffers::scanBuilder post(builder);
          post.add_contents_type(CadsFlatbuffers::scan_tables_profile_array);
          post.add_contents(send.Union());
          auto bytes = post.Finish();

          builder.Finish(bytes);

          auto [terminate,sent_rows] = send_bytes.resume({{builder.GetBufferPointer(),builder.GetBufferPointer()+builder.GetSize()},profiles_flat.size()});
          profiles_flat.clear(); builder.Clear();
          if(terminate) break;
          scan.uploaded += sent_rows;
          update_scan_state(scan);
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

    return scan;
  }


  std::tuple<state::scan,bool> post_scan(state::scan scan, webapi_urls urls,std::atomic<bool> &terminate)
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
    
    auto gocator = fetch_scan_gocator(db_name);
    
    if (!gocator)
    {
      spdlog::get("cads")->info("{} fetch_scan_gocator failed - {}", __func__, db_name);
      return {scan,true};
    } 
    

    auto do_upload = std::get<1>(urls.add_belt);
    auto [conveyor, conveyor_err] = fetch_scan_conveyor(db_name);

    if (conveyor_err < 0)
    {
      spdlog::get("cads")->info("{} fetch_scan_conveyor failed - {}", __func__, db_name);
      return {scan,true};
    }

    auto url = mk_post_profile_url(scan.scanned_utc,conveyor.Site,conveyor.Name, std::get<0>(urls.add_belt));
    auto [belt, belt_err] = fetch_scan_belt(db_name);
    
    if (belt_err < 0)
    {
      spdlog::get("cads")->info("{} fetch_scan_belt failed - {}", __func__, db_name);
      return {scan,true};
    }

    post_install_table(conveyor,belt,url,do_upload);
    scan = post_profiles_table(scan,url,belt.Length / scan.cardinality, belt.WidthN, do_upload);

    bool failure = scan.uploaded != scan.cardinality;
    if (!failure && scan.remote_reg == 1)
    {
      post_register_scan_table(belt.Serial,url,do_upload);
    }

    return {scan,failure};
  }

  std::string profile_as_flatbufferstring(profile p, GocatorProperties p_g, double nan){
    using namespace flatbuffers; 
   
    FlatBufferBuilder builder(4096);

    auto short_z = z_as_int16(p.z,p_g.zResolution,p_g.zOffset);
  
    auto caas = CadsFlatbuffers::CreateCaasProfileDirect(builder,
      p_g.xOrigin, 
      p_g.zOrigin, 
      p_g.width, 
      p_g.height,
      nan,
      p.x_off,
      p_g.xResolution,
      p_g.zResolution,
      p_g.zOffset,
      &short_z);

    auto msg = CadsFlatbuffers::CreateMsg(builder,
      CadsFlatbuffers::MsgContents_CaasProfile,
      caas.Union()
    );

    builder.Finish(msg);

    return std::string((char*)builder.GetBufferPointer(),builder.GetSize());
  }

}
