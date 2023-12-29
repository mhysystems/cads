#include <scandb.h>

namespace cads
{
  std::tuple<std::string,std::tuple< std::tuple<std::string,double>
    ,std::tuple<std::string,double>
    ,std::tuple<std::string,double>
    ,std::tuple<std::string,double>>> ScanLimits::decompose(){
      using namespace std::literals;
    return {"ScanLimits",{{"ZMin"s,ZMin}, {"ZMax"s,ZMax}, {"XMin"s,XMin},{"XMax"s,XMax}}};
    }

    std::tuple<std::string,std::tuple<std::tuple<std::string,long>
      ,std::tuple<std::string,long> >> ScanMeta::decompose()
      {
         using namespace std::literals;
        return {"ScanMeta",{{"Version"s,Version}, {"ZEncoding"s,ZEncoding}}};
      }
}