#include <iostream>

#include <boost/program_options/value_semantic.hpp>
#include <boost/program_options.hpp>
#include <spdlog/spdlog.h>


#include <constants.h>
#include <cads.h>
#include <init.h>
#include <db.h>
#include <lua_script.h>


namespace po = boost::program_options;


int main(int argn, char **argv)
{

	using namespace cads;

	po::options_description desc("Allowed options");
  po::positional_options_description pos_desc;

	desc.add_options()
		("help,h", "produce help message")
		("config,c", po::value<std::string>(), "Config JSON file.")
    ("savedb,d", po::bool_switch(), "Only save one approx belt")
    ("stop,s", po::bool_switch(), "Stop Gocator")
    ("signal,e", po::bool_switch(), "Generate signal for input into python filter parameter creation")
    ("go-log,g", po::bool_switch(), "Dump Gocator Log")
    ("level,l", po::value<std::string>(), "Logging Level")
    ("db-name", po::value<std::string>(), "db file");

  pos_desc.add("db-name", 1);

	po::variables_map vm;

	try
	{
		po::store(po::command_line_parser(argn, argv).options(desc).positional(pos_desc).run(), vm);
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

  init_logs(60);
  create_default_dbs();


  return 0;

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
  }else{
	   main_script(vm["config"].as<std::string>());
  }

	return 0;
}