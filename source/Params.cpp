#include "../include/Params.hpp"
#include <boost/program_options.hpp>
#include <iostream>

namespace po = boost::program_options;


Params::Params(int argc, char* argv[])
{
    // Define options
    po::options_description desc("Allowed options");
    desc.add_options()
        ("help,h", "Show help message")
        ("port,p", po::value<int>(&port), "Specify port number. Arbitrary port will be used if none given.");

    // Parse command line
    po::variables_map vm;
    po::store(po::parse_command_line(argc, argv, desc), vm);
    po::notify(vm);

    // Handle help request
    if (vm.contains("help"))
    {
        std::cout << desc << '\n';
        no_run = true;
    }

    // Handle port argument
    if (port < 0 || port > 65535)
    {
        std::cerr << "Error: Invalid port number.\n";
        std::cerr << desc << '\n';
        no_run = true;
        invalid = true;
    }
}
