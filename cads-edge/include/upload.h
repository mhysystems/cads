#pragma once
#include <atomic>

namespace cads
{
  void upload_scan_thread(std::atomic<bool>&);
}