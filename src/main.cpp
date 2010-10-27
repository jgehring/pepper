/*
 * pepper - SCM statistics report generator
 * Copyright (C) 2010 Jonas Gehring
 *
 * file: main.cpp
 * Program entry point
 */


#include <cstdlib>
#include <iostream>

#include <signal.h>

#include "main.h"

#include "backend.h"
#include "cache.h"
#include "globals.h"
#include "options.h"
#include "report.h"


// Termination handler handlers
static void terminationHandler(int signum)
{
	Globals::terminate = true;
}

// Installs the given signal handler thread
static void installSignalHandler(void (*handler)(int))
{
	struct sigaction new_action, old_action;
	new_action.sa_handler = handler;
	sigemptyset(&new_action.sa_mask);
	new_action.sa_flags = 0;

	int signals[] = {SIGINT, SIGHUP, SIGTERM};
	for (unsigned int i = 0; i < sizeof(signals)/sizeof(signals[0]); i++) {
		sigaction(signals[i], NULL, &old_action);
		if (old_action.sa_handler != SIG_IGN) {
			sigaction(signals[i], &new_action, NULL);
		}
	}
}


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
	std::cout << "USAGE: " << PACKAGE_NAME << " [options] <repository>" << std::endl << std::endl;

	std::cout << "Valid options:" << std::endl;
	printOption("-h, --help, -?", "Output basic usage information");
	printOption("--version", "Output version information");
	printOption("--no-cache", "Disable revision cache usage");

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
#ifdef WIN32
	// On windows, we need to change the mode for stdout to binary to avoid
	// newlines being automatically translated to CRLF.
	_setmode(_fileno(stdout), _O_BINARY);
#endif // WIN32

	Options opts;
	try {
		opts.parse(argc, argv);
	} catch (const std::exception &ex) {
		std::cerr << "Error parsing arguments: " << ex.what() << std::endl;
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

	// Setup backend
	Backend *backend = Backend::backendFor(opts);
	if (backend == NULL) {
		std::cerr << "Error: No backend found for url: " << opts.repoUrl() << std::endl;
		return EXIT_FAILURE;
	}

	try {
		backend->init();
		if (opts.useCache()) {
			backend = new Cache(backend, opts);
		}
	} catch (const Pepper::Exception &ex) {
		std::cerr << "Error initializing backend: " << ex.where() << ": " << ex.what() << std::endl;
		return EXIT_FAILURE;
	}

	installSignalHandler(&terminationHandler);

//	const char *script = "reports/test.lua";
	const char *script = "reports/loc.lua";
	int ret = Report::run(script, backend);

	delete backend;
	return ret;
}
