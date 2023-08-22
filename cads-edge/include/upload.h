#pragma once
#include <atomic>

#include <constants.h>

namespace cads
{
  void upload_scan_thread(std::atomic<bool>&, UploadConfig);
}