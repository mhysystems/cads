#pragma once 

#include <tuple>
#include <string>

namespace cads
{
  struct ScanLimits{
    double Width;
    long WidthN;
    double Length;
    long LengthN;
    double ZMin;
    double ZMax;
    double XMin;

    std::tuple<std::string,std::tuple<std::tuple<std::string,double>
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