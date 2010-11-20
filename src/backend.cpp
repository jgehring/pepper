/*
 * pepper - SCM statistics report generator
 * Copyright (C) 2010 Jonas Gehring
 *
 * file: backend.cpp
 * Abstract repository backend
 */


#include <iostream>
#include <cstring>

#include "options.h"
#include "sys/fs.h"

#include "backend.h"

#ifdef USE_GIT
 #include "backends/git.h"
#endif
#ifdef USE_SUBVERSION
 #include "backends/subversion.h"
#endif


// Constructor
Backend::LogIterator::LogIterator(const std::vector<std::string> &ids)
	: m_ids(ids), m_atEnd(false)
{

}

// Destructor
Backend::LogIterator::~LogIterator()
{

}

// Returns the next revision IDs, or an empty vector
std::vector<std::string> Backend::LogIterator::nextIds()
{
	if (m_atEnd) {
		return std::vector<std::string>();
	}
	m_atEnd = true;
	return m_ids;
}

// Main thread loop
void Backend::LogIterator::run()
{
	// Do nothing here
}


// Protected constructor
Backend::Backend(const Options &options)
	: m_opts(options)
{

}

// Default virtual destructor
Backend::~Backend()
{

}

// Allocates a backend for the given options
Backend *Backend::backendFor(const Options &options)
{
	std::string forced = options.forcedBackend();
	if (!forced.empty()) {
		return backendForName(forced, options);
	}
	return backendForUrl(options.repoUrl(), options);
}

// Initializes the backend
void Backend::init()
{
	// The default implementation does nothing
}

// Gives the backend the possibility to pre-fetch the given revisions
void Backend::prefetch(const std::vector<std::string> &)
{
	// The default implementation does nothing
}

// Cleans up the backend after iteration has finished
void Backend::finalize()
{
	// The default implementation does nothing
}

// Returns a const reference to the program options
const Options &Backend::options() const
{
	return m_opts;
}

// Allocates a backend of a specific repository type
Backend *Backend::backendForName(const std::string &name, const Options &options)
{
#ifdef USE_SUBVERSION
	if (name == "svn" || name == "subversion") {
		return new SubversionBackend(options);
	}
#endif
#ifdef USE_GIT
	if (name == "git") {
		return new GitBackend(options);
	}
#endif
	return NULL;
}

// Tries to guess a backend by examining the repository URL
Backend *Backend::backendForUrl(const std::string &url, const Options &options)
{
	// An actual url?
#ifdef USE_SUBVERSION
	const char *schemes[] = {"svn://", "svn+ssh://", "http://", "https://", "file://"};
	for (int i = 0; i < sizeof(schemes)/sizeof(schemes[0]); i++) {
		if (url.compare(0, strlen(schemes[i]), schemes[i]) == 0) {
			return new SubversionBackend(options);
		}
	}
#endif

	// A local path?
#ifdef USE_GIT
	if (sys::fs::dirExists(url+"/.git")) {
		return new GitBackend(options);
	}
#endif

	// Hmm, dunno
	return NULL;
}
