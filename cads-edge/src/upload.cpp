#include <filesystem>
#include <tuple>
#include <list>
#include <future>
#include <algorithm>

#include <date/date.h>
#include <date/tz.h>
#include <spdlog/spdlog.h>

#include <db.h>
#include <constants.h>
#include <coms.h>
#include <msg.h>
#include <upload.h>

namespace 
{
  int resume_scan(cads::state::scan scan)
  {
    auto [scan_begin,path,uploaded,status] = scan;

    spdlog::get("cads")->info("Posting a scan. {}", path);
    auto err = cads::post_scan(scan);
    
    if(!err) {
      cads::delete_scan_state(path);
      std::filesystem::remove(path);
      spdlog::get("cads")->info("Removed posted scan. {}", path);
    }else{
      
    }
    return 0;
  }

}

namespace cads
{

void upload_scan_thread(cads::Io &fifo, cads::Io &next) 
{
  std::future<std::invoke_result_t<decltype(resume_scan), state::scan>> fut;
  std::list<decltype(fut)> running_uploads;

  bool loop = true;
  cads::msg m;


  auto scans = fetch_scan_state();

  // resume incomplete scan upload
  for(auto scan : scans) {
    
    auto [scan_begin,path,uploaded,status] = scans.front();
    
    if(status == 1) {
      running_uploads.push_back(std::async(resume_scan, scan));
    }
  }


  do
  {
    auto have_value = fifo.wait_dequeue_timed(m, std::chrono::seconds(60));

    if(have_value) {
      spdlog::get("cads")->info("Recieved a scan");
    }else {
      continue;
    }  
    
    auto mid = get<0>(m);
    switch (mid)
    {
      case msgid::finished:
        loop = false;
        next.enqueue(m);
        break;
      default:
        next.enqueue(m);
        break;
    }

    auto scans = fetch_scan_state();

    // restart errored uploads
    for(const auto &scan : scans) {
      auto [scan_begin,path,uploaded,status] = scan;
      if(status == 1 && running_uploads.size() == 0) {
        running_uploads.push_back(std::async(resume_scan, scan));
      }
    }


    std::remove_if(scans.begin(),scans.end(),[](auto s) { return std::get<state::scani::Status>(s) != 0;});

    // keep last scan
    while(scans.size() > 1)
    {
      auto [scan_begin,path,uploaded,status] = scans.front();
      scans.pop_front();
      
      if(status == 0) {
        delete_scan_state(path);
        std::filesystem::remove(path);
        spdlog::get("cads")->info("Removing a scan. {}", path);
      }
    }

    // remove finshed uploads
    std::erase_if(running_uploads,[](const decltype(fut) &f ){return f.wait_for(std::chrono::seconds(0)) == std::future_status::ready;});
    
    if(scans.size() > 0) {
      auto last_upload_day = cads::fetch_daily_upload();
      auto [scan_begin,path,uploaded,status] = scans.front();
      
      if( scan_begin > last_upload_day) {
        store_scan_status(1,path);
        auto next_upload_date = last_upload_day + std::chrono::days(1);
        store_daily_upload(next_upload_date);
        running_uploads.push_back(std::async(resume_scan, scans.front()));
      }
    }
  
  }while(loop);

  spdlog::get("cads")->info("Stoppping Upload Thread");
}

}