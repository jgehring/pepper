/*
 * pepper - SCM statistics report generator
 * Copyright (C) 2010-2014 Jonas Gehring
 *
 * Released under the GNU General Public License, version 3.
 * Please see the COPYING file in the source distribution for license
 * terms and conditions, or see http://www.gnu.org/licenses/.
 *
 * file: abstractcache.cpp
 * Abstract base class for revision caches
 */


#include "main.h"

#include "bstream.h"
#include "diffstat.h"
#include "logger.h"
#include "options.h"
#include "revision.h"
#include "strlib.h"
#include "utils.h"

#include "syslib/fs.h"

#include "abstractcache.h"


// Constructor
AbstractCache::AbstractCache(Backend *backend, const Options &options)
	: Backend(options), m_backend(backend)
{

}

// Destructor
AbstractCache::~AbstractCache()
{

}

// Returns a diffstat for the specified revision
Diffstat AbstractCache::diffstat(const std::string &id)
{
	if (!lookup(id)) {
		PTRACE << "Cache miss: " << id << endl;
		return m_backend->diffstat(id);
	}

	PTRACE << "Cache hit: " << id << endl;
	Revision *r = get(id);
	Diffstat stat = r->diffstat();
	delete r;
	return stat;
}

// Tells the wrapped backend to pre-fetch revisions that are not cached yet
void AbstractCache::prefetch(const std::vector<std::string> &ids)
{
	std::vector<std::string> missing;
	for (unsigned int i = 0; i < ids.size(); i++) {
		if (!lookup(ids[i])) {
			missing.push_back(ids[i]);
		}
	}

	PDEBUG << "Cache: " << (ids.size() - missing.size()) << " of " << ids.size() << " revisions already cached, prefetching " << missing.size() << endl;
	if (!missing.empty()) {
		m_backend->prefetch(missing);
	}
}

// Returns the revision data for the given ID
Revision *AbstractCache::revision(const std::string &id)
{
	if (!lookup(id)) {
		PTRACE << "Cache miss: " << id << endl;
		Revision *r = m_backend->revision(id);
		put(id, *r);
		return r;
	}

	PTRACE << "Cache hit: " << id << endl;
	return get(id);
}

// Returns the full path for a cache file for the given backend
std::string AbstractCache::cacheFile(Backend *backend, const std::string &name)
{
	std::string dir = backend->options().cacheDir() + "/" + backend->uuid();
	checkDir(dir);
	return dir + "/" + sys::fs::escape(name);
}

// Returns the current cache directory
std::string AbstractCache::cacheDir()
{
	return m_opts.cacheDir() + "/" + uuid();
}

// Ensures that the cache dir is writable and exists
void AbstractCache::checkDir(const std::string &path, bool *created)
{
	if (!sys::fs::dirExists(path)) {
		// Create the cache directory
		try {
			sys::fs::mkpath(path);
		} catch (const std::exception &ex) {
			throw PEX(str::printf("Unable to create cache directory: %s", ex.what()));
		}
		PDEBUG << "Cache: Creating cache directory '" << path << '\'' << endl;

		if (created) {
			*created = true;
		}
	} else if (created) {
		*created = false;
	}
}
