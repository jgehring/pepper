/*
 * pepper - SCM statistics report generator
 * Copyright (C) 2010-2011 Jonas Gehring
 *
 * Released under the GNU General Public License, version 3.
 * Please see the COPYING file in the source distribution for license
 * terms and conditions, or see http://www.gnu.org/licenses/.
 *
 * file: options.cpp
 * Command-line option parsing
 */


#include "main.h"

#include <cstdlib>
#include <cstring>

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
	return (value("help") == "true");
}

bool Options::versionRequested() const
{
	return (value("version") == "true");
}

bool Options::backendListRequested() const
{
	return (value("list_backends") == "true");
}

bool Options::reportListRequested() const
{
	return (value("list_reports") == "true");
}

bool Options::useCache() const
{
	return (value("cache") == "true");
}

std::string Options::cacheDir() const
{
	return value("cache_dir");
}

std::string Options::forcedBackend() const
{
	return value("backend");
}

std::string Options::repository() const
{
	return value("repository");
}

std::string Options::value(const std::string &key, const std::string &defvalue) const
{
	std::map<std::string, std::string>::const_iterator it = m_options.find(key);
	if (it != m_options.end()) {
		return it->second;
	}
	return defvalue;
}

std::map<std::string, std::string> Options::options() const
{
	return m_options;
}

std::string Options::report() const
{
	return value("report");
}

std::map<std::string, std::string> Options::reportOptions() const
{
	return m_reportOptions;
}


// Pretty-prints a help screen option
void Options::print(const std::string &option, const std::string &text, std::ostream &out)
{
	const size_t offset = 34;
	const size_t textlen = 78 - offset;
	out << "  " << option;
	if (option.length() < offset-4) {
		for (size_t i = option.length(); i < offset-2; i++) {
			out << " ";
		}
	} else {
		out << std::endl;
		for (size_t i = 0; i < offset; i++) {
			out << " ";
		}
	}

	if (text.length() < textlen) {
		out << text << std::endl;
		return;
	}

	// Word-wrap text
	std::vector<std::string> words = utils::split(text, " ");
	int pos = 0;
	for (size_t j = 0; j < words.size(); j++) {
		if (pos + words[j].length() > textlen-1) {
			out << std::endl;
			for (size_t i = 0; i < offset; i++) {
				out << " ";
			}
			pos = 0;
		}
		out << words[j] << " ";
		pos += words[j].length() + 1;
	}
	out << std::endl;
}


// Prints the main program options
void Options::printHelp(std::ostream &out)
{
	print("-h, --help, -?", "Print basic usage information", out);
	print("--version", "Print version information", out);
	print("-v, --verbose", "Increase verbosity", out);
	print("-q, --quiet", "Set verbosity to minimum", out);
	print("-bARG, --backend=ARG", "Force usage of backend named ARG", out);
	print("--no-cache", "Disable revision cache usage", out);
	out << std::endl;
	print("--list-reports", "List built-in report scrtips", out);
	print("--list-backends", "List available backends", out);
}


// Resets the options to default values
void Options::reset()
{
	m_options.clear();
	m_reportOptions.clear();

	m_options["repository"] = sys::fs::cwd();
	m_options["cache"] = "true";
#ifdef DEBUG
	Logger::setLevel(Logger::Info);
#else
	Logger::setLevel(Logger::Status);
#endif

	// TODO: Where on Windows?
	if (char *cachedir = getenv("PEPPER_CACHEDIR")) {
		m_options["cache_dir"] = std::string(cachedir);
	} else {
		m_options["cache_dir"] = utils::strprintf("%s/.%s/cache", getenv("HOME"), PACKAGE_NAME, "cache");
	}
	PDEBUG << "Default cache dir set to " << m_options["cache_dir"] << endl;
}

// The actual parsing
void Options::parse(const std::vector<std::string> &args)
{
	struct option_t {
		const char *flag, *key, *value;
	} static mainopts[] = {
		{"-?", "help", "true"},
		{"-h", "help", "true"},
		{"--help", "help", "true"},
		{"--version", "version", "true"},
		{"--no-cache", "cache", "false"},
		{"--list-backends", "list_backends", "true"},
		{"--list-reports", "list_reports", "true"}
	};

	unsigned int i = 0;
	std::string key, value;

	bool compat01 = (strncmp(PACKAGE_VERSION, "0.1", 3) == 0);

	// Parse main options
	while (i < args.size()) {
		bool ok = false;
		for (unsigned int j = 0; j < sizeof(mainopts) / sizeof(option_t); j++) {
			if (args[i] == mainopts[j].flag) {
				m_options[mainopts[j].key] = mainopts[j].value;
				ok = true;
				break;
			}
		}

		if (!ok) {
			if (args[i] == "-v" || args[i] == "--verbose") {
				Logger::setLevel(Logger::level()+1);
			} else if (args[i] == "-q" || args[i] == "--quiet") {
				Logger::setLevel(Logger::None);

			// Compability notes
			} else if (compat01 && args[i] == "--check-cache") {
				Logger::warn() << "NOTE: The --check-cache option is deprecated, plase use the cache_check report" << endl;

			} else if (parseOpt(args[i], &key, &value)) {
				if (key == "b") {
					key = "backend";
				}
				m_options[key] = value;
			} else {
				m_options["report"] = args[i];
				++i;
				break;
			}
		}
		++i;
	}

	// Parse report options
	while (i < args.size()) {
		if (parseOpt(args[i], &key, &value)) {
			m_reportOptions[key] = value;
		} else {
			break;
		}
		++i;
	}

	// Repository URL
	if (i < args.size()) {
		try {
			m_options["repository"] = sys::fs::makeAbsolute(args[i]);
		} catch (...) {
			m_options["repository"] = args[i];
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
