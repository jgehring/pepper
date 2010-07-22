/*
 * pepper - SCM statistics report generator
 * Copyright (C) 2010 Jonas Gehring
 *
 * file: backend.cpp
 * Abstract repository backend
 */


#include <sys/stat.h>

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
	// Try to guess repository type by URL
	std::string url = options.repoUrl();

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
