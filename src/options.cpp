/*
 * pepper - SCM statistics report generator
 * Copyright (C) 2010 Jonas Gehring
 *
 * file: options.cpp
 * Command-line option parsing
 */


#include "utils.h"

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
	return !(m_options["help"].empty());
}

bool Options::versionRequested() const
{
	return !(m_options["version"].empty());
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

// Resets the options to default values
void Options::reset()
{
	m_valid = true;
	m_error.clear();
	m_options.clear();
}

// The actual parsing
void Options::parse(const std::vector<std::string> &args)
{
	for (size_t i = 0; i < args.size(); i++) {
		if (args[i] == "-?" || args[i] == "-h" || args[i] == "--help") {
			putopt("help", "true");
		} else if (args[i] == "--version") {
			putopt("version", "true");
		} else if (m_options["url"].empty()) {
			m_options["url"] = args[i];
		} else {
			throw Utils::strprintf("Unkown argument %s", args[i].c_str());
		}
	}
}

// Puts two strings to the option map
void Options::putopt(const char *key, const char *value)
{
	m_options[key] = value;
}
