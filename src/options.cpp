/*
 * pepper - SCM statistics report generator
 * Copyright (C) 2010 Jonas Gehring
 *
 * file: options.cpp
 * Command-line option parsing
 */


#include <cerrno>
#include <cstdlib>

#include "utils.h"
#include "sys/fs.h"

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
bool Options::valid() const
{
	return m_valid;
}

std::string Options::error() const
{
	return m_error;
}

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

Options::AuthData Options::authData() const
{
	AuthData data;
	data.username = m_options["username"];
	data.password = m_options["password"];
	return data;
}

std::string Options::script() const
{
	return m_options["script"];
}

std::map<std::string, std::string> Options::scriptOptions() const
{
	return m_scriptOptions;
}

// Resets the options to default values
void Options::reset()
{
	m_valid = true;
	m_error.clear();
	m_options.clear();
	m_backendOptions.clear();
	m_scriptOptions.clear();

	m_options["cache"] = "true";

	// TODO: Where on Windows?
	m_options["cache_dir"] = utils::strprintf("%s/.%s/cache", getenv("HOME"), PACKAGE_NAME, "cache");
}

// The actual parsing
void Options::parse(const std::vector<std::string> &args)
{
	enum optmode { MAIN, BACKEND, SCRIPT };
	int mode = MAIN;
	std::string key;
	for (size_t i = 0; i < args.size(); i++) {
		if (args[i] == "-?" || args[i] == "-h" || args[i] == "--help") {
			m_options["help"] = "true";
			if (mode == BACKEND) {
				m_options["backend_help"] = "true";
			} else if (mode == SCRIPT) {
				m_options["script_help"] = "true";
			}
		} else if (args[i] == "--version") {
			m_options["version"] = "true";
		} else if (args[i] == "--no-cache") {
			m_options["cache"] = "false";
		} else if (mode == MAIN && !args[i].compare(0, 2, "--") && args[i].length() > 2) {
			m_options["forced_backend"] = args[i].substr(2);
			mode = BACKEND;

		} else if (mode != MAIN && args[i].length() > 1 && args[i][0] == '-') {
			key = args[i];
		} else if (!key.empty()) {
			(mode == SCRIPT ? m_scriptOptions : m_backendOptions)[key] = args[i];
			key.clear();

		} else if (m_options["script"].empty()) {
			m_options["script"] = args[i];
			mode = SCRIPT;
		} else if (m_options["url"].empty()) {
			m_options["url"] = makeAbsolute(args[i]);
		} else {
			throw PEX(utils::strprintf("Unkown argument %s", args[i].c_str()));
		}
	}
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
