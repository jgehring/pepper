/*
 * pepper - SCM statistics report generator
 * Copyright (C) 2010-2011 Jonas Gehring
 *
 * Released under the GNU General Public License, version 3.
 * Please see the COPYING file in the source distribution for license
 * terms and conditions, or see http://www.gnu.org/licenses/.
 *
 * file: backend.cpp
 * Abstract repository backend
 */


#include "main.h"

#include "options.h"

#include "backend.h"

#ifdef USE_GIT
 #include "backends/git.h"
#endif
#ifdef USE_MERCURIAL
 #include "backends/mercurial.h"
#endif
#ifdef USE_SUBVERSION
 #include "backends/subversion.h"
#endif


// Constructor
Backend::LogIterator::LogIterator(const std::vector<std::string> &ids)
	: sys::parallel::Thread(), m_ids(ids), m_atEnd(false)
{

}

// Destructor
Backend::LogIterator::~LogIterator()
{

}

// Adds the next revision IDs to the queue or returns false
bool Backend::LogIterator::nextIds(std::queue<std::string> *queue)
{
	if (m_atEnd) {
		return false;
	}

	for (unsigned int i = 0; i < m_ids.size(); i++) {
		queue->push(m_ids[i]);
	}
	m_atEnd = true;
	m_ids.clear();
	return true;
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
	return backendForUrl(options.repository(), options);
}

// Prints a listing of available backends to
void Backend::listBackends(std::ostream &out)
{
	out << "Available backends (with abbreviations):" << std::endl;
#ifdef USE_SUBVERSION
	Options::print("subversion, svn", "Subversion", out);
#endif
#ifdef USE_GIT
	Options::print("git", "Git", out);
#endif
#ifdef USE_MERCURIAL
	Options::print("mercurial, hg", "Mercurial", out);
#endif
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

// Optional diffstat filtering before it is presented to the report script
void Backend::filterDiffstat(Diffstat *)
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

// Prints a help screen
void Backend::printHelp() const
{
	// The default implementation does nothing
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
#ifdef USE_MERCURIAL
	if (name == "hg" || name == "mercurial") {
		return new MercurialBackend(options);
	}
#endif

	throw PEX(std::string("No such backend: " + name));
}

// Tries to guess a backend by examining the repository URL
Backend *Backend::backendForUrl(const std::string &url, const Options &options)
{
#ifdef USE_SUBVERSION
	if (SubversionBackend::handles(url)) {
		return new SubversionBackend(options);
	}
#endif
#ifdef USE_GIT
	if (GitBackend::handles(url)) {
		return new GitBackend(options);
	}
#endif
#ifdef USE_MERCURIAL
	if (MercurialBackend::handles(url)) {
		return new MercurialBackend(options);
	}
#endif

	// Hmm, dunno
	return NULL;
}
