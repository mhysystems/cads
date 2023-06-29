#include <iostream>

#include <boost/program_options/value_semantic.hpp>
#include <boost/program_options.hpp>
#include <spdlog/spdlog.h>


#include <constants.h>
#include <cads.h>
#include <init.h>
#include <db.h>


namespace po = boost::program_options;


int main(int argn, char **argv)
{

	using namespace cads;

	po::options_description desc("Allowed options");

	desc.add_options()
		("help,h", "produce help message")
		("config,c", po::value<std::string>(), "Config JSON file.")
    ("savedb,d", po::bool_switch(), "Only save one approx belt")
    ("stop,s", po::bool_switch(), "Stop Gocator")
    ("signal,e", po::bool_switch(), "Generate signal for input into python filter parameter creation")
    ("go-log,g", po::bool_switch(), "Dump Gocator Log")
    ("level,l", po::value<std::string>(), "Logging Level")
    ("remote-config,r",po::bool_switch(),"Wait for remote config");

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
    init_logs(60);
    stop_gocator();
    drop_logs();
    return 0;
  }

  init_logs(60); // Must be before init_config
	
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


  create_default_dbs();

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
  }else if(vm["signal"].as<bool>()) {
    generate_signal();
  }else if(vm["go-log"].as<bool>()) {
    dump_gocator_log();
  }else if(vm["remote-config"].as<bool>()) {
    cads_remote_main();
  }else{
	  cads_local_main(vm["config"].as<std::string>());
  }

  drop_logs();
	return 0;
}