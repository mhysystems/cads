#include <scan.h>

namespace cads
{
  std::tuple<std::string,std::tuple<std::tuple<std::string,double>
    ,std::tuple<std::string,long>
    ,std::tuple<std::string,double>
    ,std::tuple<std::string,long>
    ,std::tuple<std::string,double>
    ,std::tuple<std::string,double>,
    std::tuple<std::string,double>>> ScanLimits::decompose(){
      using namespace std::literals;
    return {"ScanLimits",{{"Width"s,Width}, {"WidthN"s,WidthN},{"Length"s,Length}, {"LengthN"s,LengthN}, {"ZMin"s,ZMin}, {"ZMax"s,ZMax}, {"XMin"s,XMin}}};
    }
}