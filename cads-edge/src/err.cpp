#include <err.h>
#include <optional>

namespace cads 
{
  namespace errors
  {
    struct Err::Impl
    {
      const char* file;
      const char* func;
      int line;
      std::optional<Err> child;
    };

    Err::Err(const char* file, const char* func, int line) : pImpl{std::make_unique<Impl>(Impl{.file = file, .func = func, .line = line})} {}
    Err::~Err(){}

    Err::Err(const Err& e) : pImpl{std::make_unique<Impl>(*e.pImpl.get())} {}
  }

}