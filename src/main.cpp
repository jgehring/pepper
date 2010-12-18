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
	Options::print("-h, --help, -?", "Print basic usage information");
	Options::print("--version", "Print version information");
	Options::print("--no-cache", "Disable revision cache usage");
	Options::print("--check-cache", "Run cache check");

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
	} else if (opts.repoUrl().empty() || (!opts.checkCache() && opts.script().empty())) {
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

			// Simple cache check?
			if (opts.checkCache()) {
				try {
					cache->check();
					return EXIT_SUCCESS;
				} catch (const Pepper::Exception &ex) {
					std::cerr << "Error checking cache: " << ex.where() << ": " << ex.what() << std::endl;
					Logger::flush();
					return EXIT_FAILURE;
				}
			}

			cache->init();
		}
	} catch (const Pepper::Exception &ex) {
		std::cerr << "Error initializing backend: " << ex.where() << ": " << ex.what() << std::endl;
		Logger::flush();
		return EXIT_FAILURE;
	}

	sighandler.start();

	int ret;
	try {
		ret = Report::run(opts.script(), backend);
	} catch (const Pepper::Exception &ex) {
		std::cerr << "Recevied exception while running report:" << std::endl;
		std::cerr << "  what(): " << ex.what() << std::endl;
		std::cerr << "  where(): " << ex.where() << std::endl;
		ret = EXIT_FAILURE;
	} catch (const std::exception &ex) {
		std::cerr << "Recevied exception while running report:" << std::endl;
		std::cerr << "  what(): " << ex.what() << std::endl;
		ret = EXIT_FAILURE;
	}

	delete backend; // This will also flush the cache
	Logger::flush();

	// Close log files
	for (unsigned int i = 0; i < streams.size(); i++) {
		streams[i]->close();
		delete streams[i];
	}
	return ret;
}
