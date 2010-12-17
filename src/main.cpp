/*
 * pepper - SCM statistics report generator
 * Copyright (C) 2010 Jonas Gehring
 *
 * file: main.cpp
 * Program entry point
 */


#include <cassert>
#include <cstdlib>
#include <fstream>
#include <iostream>

#include "main.h"

#include "backend.h"
#include "cache.h"
#include "globals.h"
#include "logger.h"
#include "options.h"
#include "report.h"
#include "signalhandler.h"
#include "utils.h"


// Prints program usage information
static void printHelp(const Options &opts)
{
	std::cout << "USAGE: " << PACKAGE_NAME << " [opts.] [backend] [backend opts.] <script> [script opts.] <repository>" << std::endl << std::endl;

	std::cout << "Main options:" << std::endl;
	utils::printOption("-h, --help, -?", "Print basic usage information");
	utils::printOption("--version", "Print version information");
	utils::printOption("--no-cache", "Disable revision cache usage");

	if (!opts.repoUrl().empty() || !opts.forcedBackend().empty()) {
		std::cout << std::endl;
		Backend *backend = Backend::backendFor(opts);
		if (backend == NULL) {
			std::cout << "Error: Unkown backend " << opts.forcedBackend() << std::endl;
		} else {
			std::cout << "Options for the " << backend->name() << " backend:" << std::endl;
			backend->printHelp();
		}
		delete backend;
	}
	if (!opts.script().empty()) {
		std::cout << std::endl;
		try {
			Report::printHelp(opts.script());
		} catch (const Pepper::Exception &ex) {
			std::cerr << ex.where() << ": " << ex.what() << std::endl;
			return;
		}
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

// Configures the global logging streams for the given options
static void setupLogger(std::vector<std::ofstream *> *streams, const Options &)
{
	// Note that the log level has already been set by Options::parse()
#ifdef DEBUG
	// In debug mode, write log data to files if the log level is not high enough
	std::string files[Logger::NumLevels] = {"", "error.out", "status.out", "info.out", "debug.out", "trace.out"};
	for (int i = Logger::level()+1; i < Logger::NumLevels; i++) {
		if (!files[i].empty()) {
			std::ofstream *out = new std::ofstream();
			out->open(files[i].c_str(), std::ios::out);
			assert(out->good());
			streams->push_back(out);

			Logger::setOutput(*out, i);
		}
	}

	// Turn log level to maximum
	Logger::setLevel(Logger::NumLevels);
#endif
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

	std::vector<std::ofstream *> streams;
	setupLogger(&streams, opts);

	if (opts.helpRequested()) {
		printHelp(opts);
		return EXIT_SUCCESS;
	} else if (opts.versionRequested()) {
		printVersion();
		return EXIT_SUCCESS;
	} else if (opts.repoUrl().empty() || opts.script().empty()) {
		printHelp(opts);
		return EXIT_FAILURE;
	}

	// Setup backend
	Backend *backend = Backend::backendFor(opts);
	if (backend == NULL) {
		std::cerr << "Error: No backend found for url: " << opts.repoUrl() << std::endl;
		return EXIT_FAILURE;
	}

	// Setup signal handler thread
	SignalHandler sighandler;

	try {
		backend->init();
		if (opts.useCache()) {
			Cache *cache = new Cache(backend, opts);
			sighandler.setCache(cache);
			backend = cache;
		}
	} catch (const Pepper::Exception &ex) {
		std::cerr << "Error initializing backend: " << ex.where() << ": " << ex.what() << std::endl;
		Logger::flush();
		return EXIT_FAILURE;
	}

	sighandler.start();
	int ret = Report::run(opts.script(), backend);

	delete backend;
	Logger::flush();

	// Close log files
	for (unsigned int i = 0; i < streams.size(); i++) {
		streams[i]->close();
		delete streams[i];
	}
	return ret;
}
