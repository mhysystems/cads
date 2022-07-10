#pragma once

#include <cstddef>
#include <string>

namespace cads {
  void init_logs(size_t log_len,size_t flush);
  void drop_logs();
  std::string slurpfile(const std::string_view path, bool binaryMode = true);
}