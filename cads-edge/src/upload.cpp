#include <filesystem>
#include <tuple>
#include <list>
#include <future>
#include <chrono>

#include <date/date.h>
#include <date/tz.h>

#include <db.h>
#include <constants.h>
#include <coms.h>
#include <msg.h>

namespace cads
{

bool alarm() {

   auto next_upload_date = fetch_daily_upload();
  auto now = date::current_zone()->to_local(std::chrono::system_clock::now());
          auto dbg = date::format("%FT%T", now);
          auto dbg2 = date::format("%FT%T", next_upload_date);
  return now >= next_upload_date;

}

int resume_scan(std::string scan_name)
{
  auto [t,err] = post_scan(scan_name);
  delete_scan_state(scan_name);
  return 0;
}

void upload_scan_thread(moodycamel::BlockingReaderWriterQueue<msg> &fifo) 
{
  std::future<std::invoke_result_t<decltype(resume_scan), std::string >> fut;
  std::list<decltype(fut)> running_uploads;

  bool loop = true;
  cads::msg m;

  do
  {

    auto scans = fetch_scan_state();
    // keep last scan
    while(scans.size() > 1)
    {
      auto [scan_begin,path,uploaded] = scans.front();
      scans.pop_front();
      
      if(uploaded == 0) {
       
        delete_scan_state(path);
        std::filesystem::remove(path);
      }else {
        running_uploads.push_back(std::async(resume_scan, path));
      }
    }

    // remove finshed uploads
    std::erase_if(running_uploads,[](const decltype(fut) &f ){return f.wait_for(std::chrono::seconds(0)) == std::future_status::ready;});

    
    if(scans.size() > 0 && true) {
      auto [scan_begin,path,uploaded] = scans.front();
      running_uploads.push_back(std::async(resume_scan, path));
    }

    fifo.wait_dequeue(m);

      switch (get<0>(m))
      {
        case msgid::finished:
          loop = false;
          break;
        default:
          break;
      }
  
  }while(loop);

}

}