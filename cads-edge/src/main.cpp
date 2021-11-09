#include "cads.h"
#include "json.hpp"
#include "db.h"

#include <boost/program_options/value_semantic.hpp>
#include <boost/program_options.hpp>

#include <opencv2/core/mat.hpp>

#include <iostream>

namespace po = boost::program_options;

using json = nlohmann::json;
json global_config;

int main(int argn, char **argv)
{
	using namespace cads;

	po::options_description desc("Allowed options");

	desc.add_options()
		("help,h", "produce help message")
		("config,c", po::value<std::string>(), "Config JSON file.");

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
		cads::create_db();
	}
	else
	{
		using namespace std;
		cout << "Config File required." << endl;
		cout << desc << endl;
		return EXIT_FAILURE;
	}

	process_flatbuffers();

	return 0;
}