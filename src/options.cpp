/*
 * pepper - SCM statistics report generator
 * Copyright (C) 2010 Jonas Gehring
 *
 * file: options.cpp
 * Command-line option parsing
 */


#include <cerrno>
#include <cstdlib>

#include "logger.h"
#include "utils.h"

#include "syslib/fs.h"

#include "options.h"


// Constructor
Options::Options()
{
	reset();
}

// Option parsing
void Options::parse(int argc, char **argv)
{
	// Convert to std::string for convenience, skip program name
	std::vector<std::string> args;
	for (int i = 1; i < argc; i++) {
		args.push_back(argv[i]);
	}
	parse(args);
}

// Queries
bool Options::helpRequested() const
{
	return m_options["help"] == "true";
}

bool Options::backendHelpRequested() const
{
	return m_options["backend_help"] == "true";
}

bool Options::scriptHelpRequested() const
{
	return m_options["script_help"] == "true";
}

bool Options::versionRequested() const
{
	return m_options["version"] == "true";
}

bool Options::useCache() const
{
	return m_options["cache"] == "true";
}

std::string Options::cacheDir() const
{
	return m_options["cache_dir"];
}

std::string Options::forcedBackend() const
{
	return m_options["forced_backend"];
}

std::string Options::repoUrl() const
{
	return m_options["url"];
}

std::map<std::string, std::string> Options::backendOptions() const
{
	return m_scriptOptions;
}

std::string Options::script() const
{
	return m_options["script"];
}

std::map<std::string, std::string> Options::scriptOptions() const
{
	return m_scriptOptions;
}

// Pretty-prints a help screen option
void Options::print(const std::string &option, const std::string &text)
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


// Resets the options to default values
void Options::reset()
{
	m_options.clear();
	m_backendOptions.clear();
	m_scriptOptions.clear();

	m_options["cache"] = "true";
#ifdef DEBUG
	Logger::setLevel(Logger::Info);
#else
	Logger::setLevel(Logger::None);
#endif

	// TODO: Where on Windows?
	m_options["cache_dir"] = utils::strprintf("%s/.%s/cache", getenv("HOME"), PACKAGE_NAME, "cache");
	PDEBUG << "Default cache dir set to " << m_options["cache_dir"] << endl;
}

// The actual parsing
void Options::parse(const std::vector<std::string> &args)
{
	enum optmode { MAIN, BACKEND, SCRIPT, URL };
	int mode = URL;
	std::string key, value;
	for (int i = (int)args.size()-1; i >= 0; i--) {
		if (args[i] == "-?" || args[i] == "-h" || args[i] == "--help") {
			m_options["help"] = "true";
		} else if (args[i] == "--version") {
			m_options["version"] = "true";
		} else if (mode == URL) {
			m_options["url"] = makeAbsolute(args[i]);
			--mode;
		} else if (mode == SCRIPT) {
			if (parseOpt(args[i], &key, &value)) {
				m_scriptOptions[key] = value;
			} else {
				m_options["script"] = args[i];
				--mode;
			}

		// Main options
		} else if (args[i] == "--no-cache") {
			m_options["cache"] = "false";
		} else if (args[i] == "-v" || args[i] == "--verbose") {
			Logger::setLevel(Logger::level()+1);
		} else if (args[i] == "-q" || args[i] == "--quiet") {
			Logger::setLevel(Logger::None);
		} else if (mode == BACKEND) {
			if (parseOpt(args[i], &key, &value)) {
				m_backendOptions[key] = value;
			} else {
				m_options["forced_backend"] = args[i];
				--mode;
			}
		}
	}
}

// Parses a single option
bool Options::parseOpt(const std::string &arg, std::string *key, std::string *value)
{
	/*
	 * Valid option syntax:
	 *    -f
	 *    --flag
	 *    -fvalue
	 *    --flag=value
	 */

	if (!arg.compare(0, 2, "--") && arg.find("=") != std::string::npos && arg.find("=") > 2) {
		std::vector<std::string> parts = utils::split(arg, "=");
		*key = parts[0].substr(2);
		*value = parts[1];
		return true;
	} else if (!arg.compare(0, 2, "--")) {
		*key = arg.substr(2);
		*value = std::string();
		return true;
	} else if (!arg.compare(0, 1, "-") && arg.length() > 2) {
		*key = arg.substr(1, 1);
		*value = arg.substr(2);
		return true;
	} else if (!arg.compare(0, 1, "-") && arg.length() == 2) {
		*key = arg.substr(1);
		*value = std::string();
		return true;
	}
	return false;
}

// Converts a possibly relative path to an absolute one
std::string Options::makeAbsolute(const std::string &path)
{
	// Check for a URL scheme
	if (path.empty() || path.find("://") != std::string::npos) {
		return path;
	}

	// Make path absolute
	std::string abspath(path);
	if (abspath[0] != '/') {
		char cwd[FILENAME_MAX];
		if (!getcwd(cwd, sizeof(cwd))) {
			throw PEX(utils::strprintf("Unable to determine current directory (%d)", errno));
		}
		abspath = std::string(cwd) + "/" + path;
	}

	return sys::fs::canonicalize(abspath);
}
