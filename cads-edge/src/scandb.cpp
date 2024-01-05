#include <scandb.h>

namespace cads
{
  std::tuple<std::string,std::tuple< std::tuple<std::string,double>
    ,std::tuple<std::string,double>
    ,std::tuple<std::string,double>
    ,std::tuple<std::string,double>>> ScanLimits::decompose(){
      using namespace std::literals;
    return {"Limits",{{"ZMin"s,ZMin}, {"ZMax"s,ZMax}, {"XMin"s,XMin},{"XMax"s,XMax}}};
    }

    std::tuple<std::string,std::tuple<std::tuple<std::string,int64_t>
      ,std::tuple<std::string,int64_t> >> ScanMeta::decompose()
      {
        using namespace std::literals;
        return {"Meta",{{"Version"s,Version}, {"ZEncoding"s,ZEncoding}}};
      }
}