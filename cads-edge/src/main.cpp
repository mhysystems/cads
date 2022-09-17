#include <iostream>

#include <boost/program_options/value_semantic.hpp>
#include <boost/program_options.hpp>
#include <spdlog/spdlog.h>


#include <constants.h>
#include <cads.h>
#include <init.h>


namespace po = boost::program_options;


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
    ("upload,u", po::bool_switch(), "Upload Profile Only")
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
    init_logs(1024*1024,1);
    stop_gocator();
    return 0;
  }

	if (vm.count("config") > 0)
	{
		auto f = vm["config"].as<std::string>();
    init_config(f);
	}
	else
	{
		using namespace std;
		cout << "Config File required." << endl;
		cout << desc << endl;
		return EXIT_FAILURE;
	}

  if(global_config.find("log_lengthMiB") != global_config.end()) {
    init_logs(global_config["log_lengthMiB"].get<size_t>() * 1024 * 1024,60);
  }else {
    init_logs(5 * 1024 * 1024,60);
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
  
  if(vm["upload"].as<bool>()) {
    upload_profile_only();
  }else if(vm["savedb"].as<bool>()) {
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