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
  
  int resume_scan(cads::state::scan scan)
  {
    spdlog::get("cads")->info("Posting a scan. {}", scan.db_name);
    auto err = cads::post_scan(scan);
    
    if(!err) {
      cads::delete_scan_state(scan);
      std::filesystem::remove(scan.db_name);
      spdlog::get("cads")->info("Removed posted scan. {}", scan.db_name);
    }

    return 0;
  }

   std::vector<std::deque<cads::state::scan>> partiton(std::deque<cads::state::scan> scans) {

    if(scans.size() < 2) return {scans};

    auto first = scans.front();
    
    auto it = std::partition(scans.begin(), scans.end(), [=](cads::state::scan e) {return e.status == first.status;});

    std::vector<std::deque<cads::state::scan>> rtn{{std::begin(scans),it}};

    if(it != scans.end())
    {
      auto remainder = partiton({it,std::end(scans)});
      rtn.insert(rtn.end(), remainder.begin(),remainder.end());
    }
    
    return rtn;
  }
  
  cads::state::scan last_upload(std::deque<cads::state::scan> scans) {
    using namespace std;

    auto i = min_element(begin(scans),end(scans),[](cads::state::scan a,cads::state::scan b){ return a.scanned_utc < b.scanned_utc;});

    return *i;
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
        if(scan.status == 0 && scan.db_name != last.db_name) {
          std::filesystem::remove(scan.db_name);
          cads::delete_scan_state(scan);
        }else {
          filtered_scans.push_back(scan);
        }
      }

      pscans = filtered_scans;
    }

    return partitioned_scans;
  }

}

namespace cads
{

void upload_scan_thread(std::atomic<bool> &terminate) 
{

  do
  {
   // std::this_thread::sleep_for(std::chrono::seconds(60));
      
    auto scans = fetch_scan_state();

    if(scans.size() == 0) continue;

    auto partitioned_scans = ::remove_all_incomplete(::partiton(scans));

    for(auto pscans : partitioned_scans) {
      auto last = ::last_upload(pscans);

      for(auto scan : pscans) {
        if(scan.scanned_utc >= last.scanned_utc && scan.status == 1) {
          scan.status = 2;
          scan.scanned_utc += constants_upload.Period;
          store_scan_state(scan);
          scan.scanned_utc -= constants_upload.Period;
          resume_scan(scan);
        }
      }
    }

  }while(!terminate);

  spdlog::get("cads")->info("Stoppping Upload Thread");
}

}