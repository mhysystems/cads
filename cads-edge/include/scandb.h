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
    long Version;
    long ZEncoding;

    std::tuple<std::string,std::tuple<std::tuple<std::string,long>
      ,std::tuple<std::string,long> >> decompose();
  };

}