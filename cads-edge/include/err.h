#pragma once

#include <memory>

namespace cads
{
  namespace errors
  {

    struct Err
    {
      Err(const char*,const char*,int);
      Err(const Err&);
      virtual ~Err();

      private:
      struct Impl;
      Err() = delete;
      Err& operator=(const Err&) = delete;

      std::unique_ptr<Impl> pImpl;

    };
  }
}