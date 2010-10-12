/*
 * pepper - SCM statistics report generator
 * Copyright (C) 2010 Jonas Gehring
 *
 * file: backend.cpp
 * Abstract repository backend
 */


#include <sys/stat.h>
#include <iostream>

#include "options.h"

#include "backend.h"

#ifdef USE_GIT
 #include "backends/git.h"
#endif
#ifdef USE_SUBVERSION
 #include "backends/subversion.h"
#endif


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
#endif

	// A local path?
#ifdef USE_GIT
	struct stat statbuf;
	if (stat((url+"/.git").c_str(), &statbuf) == 0 && S_ISDIR(statbuf.st_mode)) {
		return new GitBackend(options);
	}
#endif

	// Hmm, dunno
	return NULL;
}
