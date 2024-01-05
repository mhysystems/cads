#pragma once 

#include <tuple>
#include <string>

namespace cads
{
  struct ScanLimits{
    double ZMin;
    double ZMax;
    double XMin;
    double XMax;

    std::tuple<std::string,std::tuple<std::tuple<std::string,double>
      ,std::tuple<std::string,double>
      ,std::tuple<std::string,double>
      ,std::tuple<std::string,double>>> decompose();
  };

  struct ScanMeta{
    int64_t Version;
    int64_t ZEncoding;

    std::tuple<std::string,std::tuple<std::tuple<std::string,int64_t>
      ,std::tuple<std::string,int64_t> >> decompose();
  };

}