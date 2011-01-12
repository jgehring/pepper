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
#include "logger.h"
#include "options.h"
#include "report.h"
#include "utils.h"

#include "syslib/sigblock.h"


// Signal handler
struct SignalHandler : public sys::sigblock::Handler
{
	SignalHandler(Cache *cache = NULL) : cache(cache) { }

	void operator()(int signum)
	{
		if (cache) {
			Logger::status() << "Catched signal " << signum << ", flushing cache" << endl;
			cache->flush();
		}
	}

	Cache *cache;
};


// Prints a short footer for help screens and listings
static void printFooter()
{
	std::cout << std::endl;
	std::cout << "Report bugs to " << "<" << PACKAGE_BUGREPORT ">" << std::endl;
}

// Prints program usage information
static void printHelp(const Options &opts)
{
	std::cout << "USAGE: " << PACKAGE_NAME << " [options] [report] [report options] <repository>" << std::endl << std::endl;

	std::cout << "Main options:" << std::endl;
	Options::print("-h, --help, -?", "Print basic usage information");
	Options::print("--version", "Print version information");
	Options::print("-v, --verbose", "Increase verbosity");
	Options::print("-q, --quiet", "Set verbosity to minimum");
	Options::print("-bARG, --backend=ARG", "Force usage of backend named ARG");
	Options::print("--no-cache", "Disable revision cache usage");
	std::cout << std::endl;
	Options::print("--list-reports", "List built-in report scrtips");
	Options::print("--list-backends", "List available backends");
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
			report::printHelp(opts.script());
		} catch (const Pepper::Exception &ex) {
			std::cerr << ex.where() << ": " << ex.what() << std::endl;
			return;
		}
	}

	printFooter();
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
	std::string files[Logger::NumLevels] = {"", "error.out", "warn.out", "status.out", "info.out", "debug.out", "trace.out"};
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
#else
	(void)streams; // No compiler warnings, please
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

	// Print requested help screens or listings
	if (opts.helpRequested()) {
		printHelp(opts);
		return EXIT_SUCCESS;
	} else if (opts.versionRequested()) {
		printVersion();
		return EXIT_SUCCESS;
	} else if (opts.backendListRequested()) {
		Backend::listBackends();
		printFooter();
		return EXIT_SUCCESS;
	} else if (opts.scriptListRequested()) {
		report::listReports();
		printFooter();
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

	SignalHandler sighandler;

	try {
		if (opts.useCache()) {
			Cache *cache = new Cache(backend, opts);
			backend = cache;
			sighandler.cache = cache;

			cache->init();

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
		} else {
			backend->init();
		}
	} catch (const Pepper::Exception &ex) {
		std::cerr << "Error initializing backend: " << ex.where() << ": " << ex.what() << std::endl;
		Logger::flush();
		return EXIT_FAILURE;
	}

	int signums[] = {SIGINT, SIGTERM};
	sys::sigblock::block(2, signums, &sighandler);

	int ret;
	try {
		ret = report::run(opts.script(), backend);
	} catch (const Pepper::Exception &ex) {
		std::cerr << "Recieved exception while running report:" << std::endl;
		std::cerr << "  what(): " << ex.what() << std::endl;
		std::cerr << "  where(): " << ex.where() << std::endl;
		ret = EXIT_FAILURE;
	} catch (const std::exception &ex) {
		std::cerr << "Recieved exception while running report:" << std::endl;
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
