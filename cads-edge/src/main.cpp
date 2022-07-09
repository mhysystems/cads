#include <iostream>
#include <algorithm>
#include <cctype>
#include <fstream>

#include <boost/program_options/value_semantic.hpp>
#include <boost/program_options.hpp>
#include <spdlog/spdlog.h>
#include <spdlog/sinks/rotating_file_sink.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/sinks/dup_filter_sink.h>

#include <constants.h>
#include "cads.h"


namespace po = boost::program_options;

void init_globals(size_t log_len,size_t flush) {
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

std::string slurpfile(const std::string_view path, bool binaryMode = true)
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


int main(int argn, char **argv)
{

	using namespace cads;

	po::options_description desc("Allowed options");

	desc.add_options()
		("help,h", "produce help message")
		("config,c", po::value<std::string>(), "Config JSON file.")
    ("savedb,d", po::bool_switch(), "Only save one approx belt")
    ("once,o", po::bool_switch(), "Run once")
    ("stop,s", po::bool_switch(), "Stop Gocator")
    ("signal,e", po::bool_switch(), "Generate signal for input into python filter parameter creation")
    ("level,l", po::value<std::string>(), "Logging Level");

	po::variables_map vm;

	try
	{
		po::store(po::command_line_parser(argn, argv).options(desc).run(), vm);
		po::notify(vm);
	}
	catch (std::exception &e)
	{
		using namespace std;
		cout << endl
				<< e.what() << endl;
		cout << desc << endl;
		return EXIT_FAILURE;
	}


  if(vm["stop"].as<bool>()) {
    using namespace std;
    cout << "Only stopping gocator" << endl;
    stop_gocator();
    return 0;
  }

	if (vm.count("config") > 0)
	{
		auto f = vm["config"].as<std::string>();
		auto json = slurpfile(f);
		global_config = nlohmann::json::parse(json);
	}
	else
	{
		using namespace std;
		cout << "Config File required." << endl;
		cout << desc << endl;
		return EXIT_FAILURE;
	}

  if(global_config.find("log_lengthMiB") != global_config.end()) {
    init_globals(global_config["log_lengthMiB"].get<size_t>() * 1024 * 1024,60);
  }else {
    init_globals(5 * 1024 * 1024,60);
  }

  if(vm.count("level") > 0) {
    std::string l = vm["level"].as<std::string>();
    std::transform(l.begin(), l.end(), l.begin(),[](unsigned char c){ return std::tolower(c); });

    if(l == "info") {
      spdlog::set_level(spdlog::level::info);
    }else if(l == "debug") {
      spdlog::set_level(spdlog::level::debug);
    }

  }

  if(vm["savedb"].as<bool>()) {
    store_profile_only();
  }else if(vm["once"].as<bool>()) {
    process();
  }else if(vm["signal"].as<bool>()) {
    generate_signal();
  }else{
	  process();
  }

	return 0;
}