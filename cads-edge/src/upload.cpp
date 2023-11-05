#include <filesystem>
#include <tuple>
#include <algorithm>
#include <span>

#include <date/date.h>
#include <date/tz.h>
#include <spdlog/spdlog.h>

#include <db.h>
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
  
  int resume_scan(cads::state::scan scan, cads::webapi_urls urls, std::atomic<bool> &terminate)
  {
    spdlog::get("cads")->info("{{func = {}, msg = 'Posting a scan. {}'}}", __func__,scan.db_name);
    
    if(scan.status != 2) return 0;
    
    auto [updated_scan,err] = cads::post_scan(scan, urls,terminate);
    
    if(!err) {
      updated_scan.status = 3;
      cads::update_scan_state(updated_scan);
    }

    return 0;
  }

  std::vector<std::deque<cads::state::scan>> partiton(std::deque<cads::state::scan> scans, std::function<bool(cads::state::scan,cads::state::scan)> part = [](cads::state::scan e,cads::state::scan t) {return e.conveyor_id == t.conveyor_id;}) {

    using namespace std::placeholders;

    if(scans.size() < 2) return {scans};

    auto first = scans.front();
    auto pred = std::bind(part, first, _1);
    
    auto it = std::partition(scans.begin(), scans.end(), pred);

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

    std::deque<cads::state::scan>::iterator it;

    for(auto i : std::array<int64_t,3>{2,3,1}) {
      it = std::partition(scans.begin(), scans.end(), [=](cads::state::scan e) {return e.status == i;});
      if(it != scans.begin()) break;
    }
    
    for(auto scan = scans.begin(); scan < it; ++scan) {
      
      if(scan->scanned_utc > max.scanned_utc && scan->status >= max.status) {
        max = *scan;
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

void upload_scan_thread(std::atomic<bool> &terminate, UploadConfig config) 
{

  do
  {
    std::this_thread::sleep_for(std::chrono::seconds(15));
      

    auto scans = fetch_scan_state();

    if(scans.size() == 0) continue;

    for(auto rscans : ::partiton(scans,[](state::scan a,state::scan b){return a.remote_reg == b.remote_reg;})) 
    {
      auto partitioned_scans = ::remove_all_incomplete(::partiton(rscans));

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
          resume_scan(scan,config.urls,terminate);
        }
        else if(scan.status == 1 && scan.scanned_utc >= (latest.scanned_utc + config.Period) ) 
        {
          scan.status = 2;
          if(update_scan_state(scan))
          {
            latest = scan;
            resume_scan(scan,config.urls,terminate);
          }
        }
        else if(scan.db_name != latest.db_name && scan.scanned_utc < (latest.scanned_utc + config.Period) && scan.status != 2 && scan.status != 0) 
        {
          delete_scan(scan);
        }
      }
    }
    }
  }while(!terminate);

  spdlog::get("cads")->info("{{func = {}, msg = 'Exiting'}}", __func__);
}

}

#if 0

  std::vector<std::deque<cads::state::scan>> partition_scans(std::deque<cads::state::scan> scans, std::function<bool(cads::state::scan)> partion)
  {
    if(scans.size() < 2) return {scans};

    auto it = std::partition(scans.begin(), scans.end(), partion);

    if(it == scans.begin() || it == scans.end())
    {
      return {scans};
    }
    else
    {
      return {{std::begin(scans),it},{it,std::end(scans)}};
    }
  }

  std::deque<cads::state::scan> choose(std::vector<std::deque<cads::state::scan>> partitioned_scans, std::function<bool(cads::state::scan)> choice)
  {
    for(auto scan : partitioned_scans) {
      if(!scan.empty() && choice(scan.front())) return scan;
    }

    return {};
  }

  std::vector<std::deque<cads::state::scan>> partiton_status_1(std::deque<cads::state::scan> scans, date::utc_clock::time_point start, std::chrono::seconds period) 
  {
    namespace views = std::ranges::views;

    if(scans.size() < 2 ) return {scans};

    auto status_1 = scans | views::filter([](cads::state::scan e){ return e.status == 1;});

    auto parts = partition_scans({status_1.begin(), status_1.end()}, [=](cads::state::scan e) {return e.scanned_utc < start;});

    std::vector<std::deque<cads::state::scan>> rtn;

    if(parts.size() == 2)
    {
      rtn.push_back(parts[0]);
      auto remainder = partiton_status_1(parts[1],start+period,period);
      rtn.insert(rtn.end(), remainder.begin(),remainder.end());
      return rtn;
    }else {
      rtn.push_back(parts[0]);
      return rtn; 
    }

  }

  void delete_old_scans_status03(std::deque<cads::state::scan> partition_conveyor_ids)
  {
    using namespace std;

    for(auto c : std::array<bool(cads::state::scan),2>{[](cads::state::scan e) {return e.status == 0;},[](cads::state::scan e) {return e.status == 3;}}) {
      auto scans_status_0 = ::choose(partition_conveyor_ids,[](cads::state::scan e) {return e.status == 0;});
      auto scans_status_0_latest = max_element(begin(scans_status_0),end(scans_status_0),[](cads::state::scan a,cads::state::scan b){ return a.scanned_utc > b.scanned_utc;});

      erase(scans_status_0,scans_status_0_latest);

      for(auto scan : scans_status_0) {
        ::delete_scan(scan);
      }
    }

  }
#endif
