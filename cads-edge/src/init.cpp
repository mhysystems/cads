
#include <array>
#include <algorithm>
#include <fstream>

#include <spdlog/spdlog.h>
#include <spdlog/sinks/rotating_file_sink.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/sinks/dup_filter_sink.h>

namespace cads {

void init_logs(size_t log_len,size_t flush) {
  using namespace std;

  array<shared_ptr<spdlog::sinks::dup_filter_sink_mt>,2> dup_filter {
    std::make_shared<spdlog::sinks::dup_filter_sink_mt>(std::chrono::seconds(5)),
    std::make_shared<spdlog::sinks::dup_filter_sink_mt>(std::chrono::seconds(5))
  };

  dup_filter[0]->add_sink(std::make_shared<spdlog::sinks::rotating_file_sink_mt>("cads.log", log_len, 1));
  dup_filter[1]->add_sink(std::make_shared<spdlog::sinks::stdout_color_sink_mt>());
  
  array<spdlog::sink_ptr,2> ms{dup_filter[0],dup_filter[1]};
  
  array<shared_ptr<spdlog::logger>,4> log_sections {
    make_shared<spdlog::logger>("cads", ms.begin(),ms.end()),
    make_shared<spdlog::logger>("gocator", ms.begin(),ms.end()),
    make_shared<spdlog::logger>("db", ms.begin(),ms.end()),  
    make_shared<spdlog::logger>("upload", ms.begin(),ms.end())
  };
  
  for(auto ls : log_sections) {
    spdlog::register_logger(ls);
  }
  
  spdlog::flush_every(std::chrono::seconds(flush));
}

void drop_logs() {
  spdlog::drop_all();
}

std::string slurpfile(const std::string_view path, bool binaryMode)
{
  std::ios::openmode openmode = std::ios::in;
  if (binaryMode)
  {
    openmode |= std::ios::binary;
  }
  std::ifstream ifs(path.data(), openmode);
  ifs.ignore(std::numeric_limits<std::streamsize>::max());
  std::string data(ifs.gcount(), 0);
  ifs.seekg(0);
  ifs.read(data.data(), data.size());
  return data;
}

}

