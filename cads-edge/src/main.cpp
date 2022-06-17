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

void init_globals() {
  std::vector<spdlog::sink_ptr> ms{std::make_shared<spdlog::sinks::rotating_file_sink_mt>("cads.log", 1024 * 1024 * 5, 1), std::make_shared<spdlog::sinks::stdout_color_sink_mt>()};
  spdlog::register_logger(std::make_shared<spdlog::logger>("cads", ms.begin(),ms.end()));
  spdlog::register_logger(std::make_shared<spdlog::logger>("gocator", ms.begin(),ms.end()));
  spdlog::register_logger(std::make_shared<spdlog::logger>("db", ms.begin(),ms.end()));
  spdlog::register_logger(std::make_shared<spdlog::logger>("upload", ms.begin(),ms.end()));
  
  spdlog::flush_every(std::chrono::seconds(60));
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
    ("experiment,e", po::bool_switch(), "Test experimental features")
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

  init_globals();

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
  }else if(vm["experiment"].as<bool>()) {
    process_experiment();
  }else if(vm["stop"].as<bool>()) {
    stop_gocator();
  }else{
	  process();
  }

	return 0;
}