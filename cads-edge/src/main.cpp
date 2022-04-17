#include "cads.h"
#include "json.hpp"

#include <boost/program_options/value_semantic.hpp>
#include <boost/program_options.hpp>

//#include <opencv2/core/mat.hpp>

#include <iostream>
#include <algorithm>
#include <cctype>

#include <spdlog/spdlog.h>

namespace po = boost::program_options;

using json = nlohmann::json;
extern json global_config;

int main(int argn, char **argv)
{
	using namespace cads;

	po::options_description desc("Allowed options");

	desc.add_options()
		("help,h", "produce help message")
		("config,c", po::value<std::string>(), "Config JSON file.")
    ("savedb,d", po::bool_switch(), "Only save one approx belt")
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
		global_config = json::parse(json);
	}
	else
	{
		using namespace std;
		cout << "Config File required." << endl;
		cout << desc << endl;
		return EXIT_FAILURE;
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
    process_flatbuffers();
    //store_profile_only();
  }else{
	  process_flatbuffers();
  }

	return 0;
}