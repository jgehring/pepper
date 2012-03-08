/*
 * pepper - SCM statistics report generator
 * Copyright (C) 2010-2012 Jonas Gehring
 *
 * Released under the GNU General Public License, version 3.
 * Please see the COPYING file in the source distribution for license
 * terms and conditions, or see http://www.gnu.org/licenses/.
 *
 * file: main.cpp
 * Program entry point
 */


#include "main.h"

#include <cassert>
#include <cstdlib>
#include <fstream>

#if defined(POS_LINUX) && defined(DEBUG)
 #include <sys/resource.h>
#endif

#include "backend.h"
#include "abstractcache.h"
#include "logger.h"
#include "options.h"
#include "report.h"

#ifdef USE_LDBCACHE
 #include "ldbcache.h"
#else
 #include "cache.h"
#endif

#include "syslib/sigblock.h"


// Signal handler
struct SignalHandler : public sys::sigblock::Handler
{
	SignalHandler(AbstractCache *cache = NULL) : cache(cache) { }

	void operator()(int signum)
	{
		// This is very unclean, but functions called from here should not be
		// blocked by logging functions. And this is more useful than turning
		// logging off.
		Logger::unlock();

		if (cache) {
			Logger::status() << "Catched signal " << signum << ", flushing cache" << endl;
			cache->flush();
		}
	}

	AbstractCache *cache;
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
	std::cout << "USAGE: " << PACKAGE_NAME << " [options] <report> [report options] [repository]" << std::endl << std::endl;

	std::cout << "Main options:" << std::endl;
	Options::printHelp();

	if (!opts.repository().empty() || !opts.forcedBackend().empty()) {
		std::cout << std::endl;
		try {
			Backend *backend = Backend::backendFor(opts);
			if (backend == NULL) {
				throw PEX("No backend found");
			}
			std::cout << "Options for the " << backend->name() << " backend:" << std::endl;
			backend->printHelp();
			delete backend;
		} catch (const std::exception &ex) {
			std::cout << "Sorry, unable to find a backend for '" << opts.repository() << "'" << std::endl;
		}
	}
	if (!opts.report().empty()) {
		std::cout << std::endl;
		try {
			Report r(opts.report());
			r.printHelp();
		} catch (const PepperException &ex) {
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
	std::cout << "Copyright (C) 2010-2012 " << "Jonas Gehring <" << PACKAGE_BUGREPORT << ">" << std::endl;
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

// Runs the program according to the given actions
int start(const Options &opts)
{
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
	} else if (opts.reportListRequested()) {
		Report::printReportListing();
		printFooter();
		return EXIT_SUCCESS;
	} else if (opts.repository().empty() || opts.report().empty()) {
		printHelp(opts);
		return EXIT_FAILURE;
	}

	// Setup backend
	Backend *backend;
	try {
		backend = Backend::backendFor(opts);
		if (backend == NULL) {
			std::cerr << "Error: No backend found for url: " << opts.repository() << std::endl;
			return EXIT_FAILURE;
		}
	} catch (const PepperException &ex) {
		std::cerr << "Error detecting repository type: " << ex.where() << ": " << ex.what() << std::endl;
		Logger::flush();
		return EXIT_FAILURE;
	}

	SignalHandler sighandler;

	AbstractCache *cache = NULL;
	try {
		if (opts.useCache()) {
			backend->init();

#ifdef USE_LDBCACHE
			cache = new LdbCache(backend, opts);
#else
			cache = new Cache(backend, opts);
#endif
			sighandler.cache = cache;

			cache->init();
		} else {
			backend->init();
		}
	} catch (const PepperException &ex) {
		std::cerr << "Error initializing backend: " << ex.where() << ": " << ex.what() << std::endl;
		Logger::flush();
		return EXIT_FAILURE;
	}

	int signums[] = {SIGINT, SIGTERM};
	sys::sigblock::block(2, signums, &sighandler);
	sys::sigblock::ignore(SIGPIPE);

	int ret;
	try {
		Report r(opts.report(), (cache ? cache : backend));
		ret = r.run();
	} catch (const PepperException &ex) {
		std::cerr << "Recieved exception while running report:" << std::endl;
		std::cerr << "  what():  " << ex.what() << std::endl;
		std::cerr << "  where(): " << ex.where() << std::endl;
		std::cerr << "  trace(): " << ex.trace() << std::endl;
		ret = EXIT_FAILURE;
	} catch (const std::exception &ex) {
		std::cerr << "Recieved exception while running report:" << std::endl;
		std::cerr << "  what(): " << ex.what() << std::endl;
		ret = EXIT_FAILURE;
	}

	delete cache; // This will also flush the cache
	delete backend;
	return ret;
}

// Program entry point
int main(int argc, char **argv)
{
#ifdef WIN32
	// On windows, we need to change the mode for stdout to binary to avoid
	// newlines being automatically translated to CRLF.
	_setmode(_fileno(stdout), _O_BINARY);
#endif // WIN32
#if defined(POS_LINUX) && defined(DEBUG)
	// Enable core dumps
	{
		struct rlimit rl;
		rl.rlim_cur = RLIM_INFINITY;
		rl.rlim_max = RLIM_INFINITY;
		setrlimit(RLIMIT_CORE, &rl);
	}
#endif

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

	int ret;
	try {
		ret = start(opts);
	} catch (const PepperException &ex) {
		std::cerr << "Recieved unhandled exception:" << std::endl;
		std::cerr << "  what():  " << ex.what() << std::endl;
		std::cerr << "  where(): " << ex.where() << std::endl;
		std::cerr << "  trace(): " << ex.trace() << std::endl;
		ret = EXIT_FAILURE;
	} catch (const std::exception &ex) {
		std::cerr << "Recieved unhandled exception:" << std::endl;
		std::cerr << "  what(): " << ex.what() << std::endl;
		ret = EXIT_FAILURE;
	}

	Logger::flush();

	// Close log files
	for (unsigned int i = 0; i < streams.size(); i++) {
		streams[i]->close();
		delete streams[i];
	}

	return ret;
}
