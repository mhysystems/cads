#include <filesystem>
#include <tuple>
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

  void delete_scan(cads::state::scan scan) 
  {
    cads::delete_scan_state(scan);
    std::filesystem::remove(scan.db_name);
    std::filesystem::remove(scan.db_name + "-shm");
    std::filesystem::remove(scan.db_name + "-wal");
    std::filesystem::remove(scan.db_name + "-journal");
    spdlog::get("cads")->info("{{func = {}, msg = 'Removed scan. {}'}}", __func__,scan.db_name);
  }
  
  int resume_scan(cads::state::scan scan)
  {
    spdlog::get("cads")->info("{{func = {}, msg = 'Posting a scan. {}'}}", __func__,scan.db_name);
    
    if(scan.status != 2) return 0;
    
    auto [updated_scan,err] = cads::post_scan(scan);
    
    if(!err) {
      updated_scan.status = 3;
      cads::update_scan_state(updated_scan);
    }

    return 0;
  }

  std::vector<std::deque<cads::state::scan>> partiton(std::deque<cads::state::scan> scans) {

    if(scans.size() < 2) return {scans};

    auto first = scans.front();
    
    auto it = std::partition(scans.begin(), scans.end(), [=](cads::state::scan e) {return e.conveyor_id == first.conveyor_id;});

    std::vector<std::deque<cads::state::scan>> rtn{{std::begin(scans),it}};

    if(it != scans.end())
    {
      auto remainder = partiton({it,std::end(scans)});
      rtn.insert(rtn.end(), remainder.begin(),remainder.end());
    }
    
    return rtn;
  }
  
  std::tuple<cads::state::scan,bool> latest_scan(std::deque<cads::state::scan> scans) {
    using namespace std;

    if(scans.size() == 0)  return {cads::state::scan{},false};

    cads::state::scan max;
    max.scanned_utc = date::utc_clock::time_point::min();
    max.status = 0;

    for(auto scan : scans) {
      
      if(scan.scanned_utc > max.scanned_utc && scan.status >= max.status) {
        max = scan;
      }
    }

    return {max,max.status != 0};
  }

    std::tuple<cads::state::scan,bool> last_filling(std::deque<cads::state::scan> scans) {
    using namespace std;
    
    auto new_end = remove_if(begin(scans),end(scans),[](cads::state::scan e) { return e.status != 0;});
    auto i = max_element(begin(scans),new_end,[](cads::state::scan a,cads::state::scan b){ return a.scanned_utc < b.scanned_utc;});

    if(i == end(scans)) {
      return {cads::state::scan{},false};
    }else {
      return {*i,i != new_end};
    }
  }

  std::vector<std::deque<cads::state::scan>> remove_all_incomplete(std::vector<std::deque<cads::state::scan>> partitioned_scans) 
  {
    
    for(auto &pscans : partitioned_scans) {

      auto [last,valid] = last_filling(pscans);

      if(!valid) continue;

      std::deque<cads::state::scan> filtered_scans;
      
      for(auto scan : pscans) {
        
        if(scan.cardinality < 1 && scan.status != 0) {
          ::delete_scan(scan);
          continue;
        }

        if(scan.status == 0 && scan.db_name == last.db_name ) continue;
        
        if(scan.status == 0 && scan.db_name != last.db_name) {
          ::delete_scan(scan);
        }else {
          filtered_scans.push_back(scan);
        }
      }

      pscans = filtered_scans;
    }

    std::erase_if(partitioned_scans,[](const std::deque<cads::state::scan> &e){return e.size() == 0;});
    return partitioned_scans;
  }

}

namespace cads
{

void upload_scan_thread(std::atomic<bool> &terminate) 
{

  do
  {
    std::this_thread::sleep_for(std::chrono::seconds(15));
      

    auto scans = fetch_scan_state();

    if(scans.size() == 0) continue;

    auto partitioned_scans = ::remove_all_incomplete(::partiton(scans));

    for(auto pscans : partitioned_scans) {
      auto [latest,valid] = ::latest_scan(pscans);

      if(!valid) {
        continue;
      }

      if(latest.status == 1) {
        latest.status = 2;
        if(!update_scan_state(latest)) {
          break;
        }

        continue;
      }

      for(auto scan : pscans) {
        
        if (!std::filesystem::exists(scan.db_name))
        {
          spdlog::get("cads")->info("{{func = {}, msg = 'File doesn't exist. {}'}}", __func__,scan.db_name);
          ::delete_scan(scan);
          continue;
        }
        
        if(scan.status == 2) 
        {
          resume_scan(scan);
        }
        else if(scan.status == 1 && scan.scanned_utc >= (latest.scanned_utc + constants_upload.Period) ) 
        {
          scan.status = 2;
          if(update_scan_state(scan))
          {
            latest = scan;
            resume_scan(scan);
          }
        }
        else if(scan.scanned_utc < (latest.scanned_utc + constants_upload.Period) && scan.status != 2 && scan.status != 0) 
        {
          delete_scan(scan);
        }
      }
    }

  }while(!terminate);

  spdlog::get("cads")->info("{{func = {}, msg = 'Exiting'}}", __func__);
}

}