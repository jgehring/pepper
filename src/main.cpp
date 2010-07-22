/*
 * pepper - SCM statistics report generator
 * Copyright (C) 2010 Jonas Gehring
 *
 * file: main.cpp
 * Program entry point
 */


#include <cstdlib>
#include <iostream>

#include "main.h"
#include "backend.h"
#include "options.h"


// Utility function for printing a help screen option
static void printOption(const std::string &option, const std::string &text)
{
	std::cout << "  " << option;
	if (option.length() < 30) {
		for (int i = option.length(); i < 32; i++) {
			std::cout << " ";
		}
	} else {
		std::cout << std::endl;
		for (int i = 0; i < 34; i++) {
			std::cout << " ";
		}
	}

	std::cout << text << std::endl;
}

// Prints program usage information
static void printHelp()
{
	std::cout << "USAGE: " << PACKAGE_NAME << " [options] <repository>" << std::endl;
	std::cout << std::endl;
	std::cout << "Valid options:" << std::endl;
	printOption("-h, --help, -?", "Output basic usage information");
	printOption("--version", "Output version information");

	std::string backends;
#ifdef USE_GIT
	backends += "--git, ";
#endif
#ifdef USE_SUBVERSION
	backends += "--svn, ";
#endif
	if (!backends.empty()) {
		backends.resize(backends.length()-2);
		printOption(backends, "Force specific backend to be used");
	}

	std::cout << std::endl;
	std::cout << "Report bugs to " << "<" << PACKAGE_BUGREPORT ">" << std::endl;
}

// Prints the program version
static void printVersion()
{
	std::cout << PACKAGE_NAME << " " << PACKAGE_VERSION << std::endl;
	std::cout << "Copyright (C) 2010 " << "Jonas Gehring <" << PACKAGE_BUGREPORT << ">" << std::endl;
	std::cout << "Released under the GNU General Public License." << std::endl;
}

// Program entry point
int main(int argc, char **argv)
{
	Options opts;
	try {
		opts.parse(argc, argv);
	} catch (const std::string &str) {
		std::cerr << "Error parsing arguments: " << str << std::endl;
		std::cerr << "Run with --help for usage information" << std::endl;
		return EXIT_FAILURE;
	}

	if (opts.helpRequested()) {
		printHelp();
		return EXIT_SUCCESS;
	} else if (opts.versionRequested()) {
		printVersion();
		return EXIT_SUCCESS;
	} else if (opts.repoUrl().empty()) {
		printHelp();
		return EXIT_FAILURE;
	}

	Backend *backend = Backend::backendFor(opts);
	if (backend == NULL) {
		std::cerr << "Error: No backend found for url: " << opts.repoUrl() << std::endl;
		return EXIT_FAILURE;
	}

	// TODO

	return EXIT_SUCCESS;
}
