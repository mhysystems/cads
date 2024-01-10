#pragma once

#include <memory>
#include <string>

namespace cads
{
  namespace errors
  {

    struct Err
    {
      Err(const char*,const char*,int, const Err&);
      Err(const char*,const char*,int);
      Err() = default;
      Err(const Err&) = default;
      virtual ~Err() = default;

      std::shared_ptr<Err> clone() const;
      std::string str() const;

      private:
      struct Impl;
      Err& operator=(const Err&) = delete;
      virtual Err* clone_impl() const;
      virtual std::string str_impl() const;

      const std::shared_ptr<Impl> pImpl = nullptr;

    };

    struct ErrCode : errors::Err
    {
      ErrCode(const char*,const char*,int,int);
      ErrCode() = default;
      virtual ~ErrCode() = default;

      operator bool();

      std::shared_ptr<ErrCode> clone() const;

      private:
      struct Impl;

      ErrCode& operator=(const ErrCode&) = delete;
      virtual ErrCode* clone_impl() const override;

      const std::shared_ptr<Impl> pImpl = nullptr;
    };
  }
}