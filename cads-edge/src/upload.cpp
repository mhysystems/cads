#include <filesystem>
#include <tuple>
#include <list>
#include <future>
#include <chrono>

#include <date/date.h>
#include <date/tz.h>
#include <spdlog/spdlog.h>

#include <db.h>
#include <constants.h>
#include <coms.h>
#include <msg.h>

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
      spdlog::get("cads")->info("Removing a posted scan. {}", path);
    }
    return 0;
  }

}

namespace cads
{

void upload_scan_thread(moodycamel::BlockingReaderWriterQueue<msg> &fifo) 
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

    auto scans = fetch_scan_state();
    // keep last scan
    while(scans.size() > 1)
    {
      auto [scan_begin,path,uploaded,status] = scans.front();
      scans.pop_front();
      
      if(uploaded == 0 && status == 0) {
       
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

    fifo.wait_dequeue(m);

      switch (get<0>(m))
      {
        case msgid::finished:
          loop = false;
          break;
        default:
          spdlog::get("cads")->info("Recieved a scan");
          break;
      }
  
  }while(loop);

}

}