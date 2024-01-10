#include <err.h>
#include <optional>

#include <fmt/core.h>


namespace cads 
{
  namespace errors
  {
    struct Err::Impl
    {
      const char* file;
      const char* func;
      const int line;
      const std::optional<std::shared_ptr<Err>> child;
    };

    Err::Err(const char* file, const char* func, int line, const Err& child) : 
      pImpl{std::make_shared<Impl>(Impl{.file = file, .func = func, .line = line, .child = child.clone()})} {}
    
    Err::Err(const char* file, const char* func, int line) : 
      pImpl{std::make_shared<Impl>(Impl{.file = file, .func = func, .line = line, .child = std::nullopt})} {}

    std::shared_ptr<Err> Err::clone() const {
      return std::shared_ptr<Err>(clone_impl());
    }


    std::string Err::str() const {
      if(pImpl->child) {
        return fmt::format("{{file = '{}', func = '{}', line = {}, child = {}}}",pImpl->file,pImpl->func,pImpl->line,(*(pImpl->child))->str());
      }else {
        return fmt::format("{{file = '{}', func = '{}', line = {}}}",pImpl->file,pImpl->func,pImpl->line);
      }
    }

    std::string Err::str_impl() const 
    {
      using namespace std::literals;
      return ""s;
    }

    Err* Err::clone_impl() const
    {
      return new Err(*this);
    }

    struct ErrCode::Impl
    {
      const int code;
    };

    ErrCode::ErrCode(const char* file, const char* func, int line, int code) : 
      Err(file,func,line), pImpl{std::make_shared<Impl>(Impl{.code = code})} {}
    
    std::shared_ptr<ErrCode> ErrCode::clone() const {
      return std::shared_ptr<ErrCode>(clone_impl());
    }

    ErrCode* ErrCode::clone_impl() const
    {
      return new ErrCode(*this);
    }

    ErrCode::operator bool()
    {
      return pImpl == nullptr;
    }

  }
}