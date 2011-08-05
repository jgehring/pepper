/*
 * pepper - SCM statistics report generator
 * Copyright (C) 2010-2011 Jonas Gehring
 *
 * Released under the GNU General Public License, version 3.
 * Please see the COPYING file in the source distribution for license
 * terms and conditions, or see http://www.gnu.org/licenses/.
 *
 * file: cache.cpp
 * Revision cache
 */


#include "main.h"

#include "bstream.h"
#include "diffstat.h"
#include "logger.h"
#include "options.h"
#include "revision.h"
#include "utils.h"

#include "syslib/datetime.h"
#include "syslib/fs.h"
#include "syslib/sigblock.h"

#include "cache.h"


#define CACHE_VERSION (uint32_t)4
#define MAX_CACHEFILE_SIZE 4194304


// Constructor
Cache::Cache(Backend *backend, const Options &options)
	: Backend(options), m_backend(backend), m_iout(NULL), m_cout(NULL),
	  m_cin(0), m_coindex(0), m_ciindex(0), m_loaded(false)
{

}

// Destructor
Cache::~Cache()
{
	flush();
}

// Returns a diffstat for the specified revision
Diffstat Cache::diffstat(const std::string &id)
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
void Cache::prefetch(const std::vector<std::string> &ids)
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
Revision *Cache::revision(const std::string &id)
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
std::string Cache::cacheFile(Backend *backend, const std::string &name)
{
	std::string dir = backend->options().cacheDir() + "/" + backend->uuid();
	checkDir(dir);
	return dir + "/" + sys::fs::escape(name);
}

// Flushes and closes the cache streams
void Cache::flush()
{
	PTRACE << "Flushing cache..." << endl;
	delete m_iout;
	m_iout = NULL;
	delete m_cout;
	m_cout = NULL;
	delete m_cin;
	m_cin = NULL;
	PTRACE << "Cache flushed" << endl;
}

// Checks if the diffstat of the given revision is already cached
bool Cache::lookup(const std::string &id)
{
	if (!m_loaded) {
		load();
	}

	return (m_index.find(id) != m_index.end());
}

// Adds the revision of the given revision to the cache
void Cache::put(const std::string &id, const Revision &rev)
{
	if (!m_loaded) {
		load();
	}

	// Defer any signals while writing to the cache
	SIGBLOCK_DEFER();

	// Add revision to cache
	std::string dir = m_opts.cacheDir() + "/" + uuid(), path;
	if (m_cout == NULL) {
		m_coindex = 0;
		do {
			path = utils::strprintf("%s/cache.%u", dir.c_str(), m_coindex);
			if (!sys::fs::fileExists(path) || sys::fs::filesize(path) < MAX_CACHEFILE_SIZE) {
				break;
			}
			++m_coindex;
		} while (true);

		delete m_cout;
		m_cout = new BOStream(path, true);
	} else if (m_cout->tell() >= MAX_CACHEFILE_SIZE) {
		delete m_cout;
		path = utils::strprintf("%s/cache.%u", dir.c_str(), ++m_coindex);
		m_cout = new BOStream(path, true);
	}

	uint32_t offset = m_cout->tell();
	MOStream rout;
	rev.write(rout);
	std::vector<char> compressed = utils::compress(rout.data());
	*m_cout << compressed;

	// Add revision to index
	if (m_iout == NULL) {
		if (sys::fs::exists(dir + "/index")) {
			m_iout = new GZOStream(dir + "/index", true);
		} else {
			m_iout = new GZOStream(dir + "/index", false);
			// Version number
			*m_iout << CACHE_VERSION;
		}
	}
	*m_iout << id;
	*m_iout << m_coindex << offset << utils::crc32(compressed);
}

// Loads a revision from the cache
Revision *Cache::get(const std::string &id)
{
	if (!m_loaded) {
		load();
	}

	std::string dir = m_opts.cacheDir() + "/" + uuid();
	std::pair<uint32_t, uint32_t> offset = m_index[id];
	std::string path = utils::strprintf("%s/cache.%u", dir.c_str(), offset.first);
	if (m_cin == NULL || offset.first != m_ciindex) {
		delete m_cin;
		m_cin = new BIStream(path);
		m_ciindex = offset.first;
		if (!m_cin->ok()) {
			throw PEX(utils::strprintf("Unable to read from cache file: %s", path.c_str()));
		}
	}
	if (!m_cin->seek(offset.second)) {
		throw PEX(utils::strprintf("Unable to read from cache file: %s", path.c_str()));
	}

	Revision *rev = new Revision(id);
	std::vector<char> data;
	*m_cin >> data;
	data = utils::uncompress(data);
	if (data.empty()) {
		throw PEX(utils::strprintf("Unable to read from cache file: %s", path.c_str()));
	}
	MIStream rin(data);
	if (!rev->load(rin)) {
		throw PEX(utils::strprintf("Unable to read from cache file: %s", path.c_str()));
	}
	return rev;
}

// Loads the index file
void Cache::load()
{
	m_index.clear();
	m_loaded = true;

	std::string path = m_opts.cacheDir() + "/" + uuid();
	PDEBUG << "Using cache dir: " << path << endl;

	bool created;
	checkDir(path, &created);
	if (created) {
		return;
	}

	// For git repositories, the hardest part is calling uuid()
	sys::datetime::Watch watch;

	GZIStream *in = new GZIStream(path+"/index");
	if (!in->ok()) {
		Logger::info() << "Cache: Empty cache for '" << uuid() << '\'' << endl;
		return;
	}

	uint32_t version;
	*in >> version;
	switch (checkVersion(version)) {
		case Abort:
			delete in;
			return;
		case Clear:
			delete in;
			clear();
			return;
		case UnknownVersion:
			throw PEX(utils::strprintf("Unknown cache version number %u", version));
		default:
			break;
	}

	Logger::status() << "Loading cache index... " << ::flush;

	std::string buffer;
	std::pair<uint32_t, uint32_t> pos;
	uint32_t crc;
	while (!in->eof()) {
		*in >> buffer;
		if (buffer.empty()) {
			break;
		}
		*in >> pos.first >> pos.second;
		*in >> crc;
		m_index[buffer] = pos;
	}

	Logger::status() << "done" << endl;
	delete in;
	Logger::info() << "Cache: Loaded " << m_index.size() << " revisions in " << watch.elapsedMSecs() << " ms" << endl;
}

// Clears all cache files
void Cache::clear()
{
	flush();

	std::string path = m_opts.cacheDir() + "/" + uuid();
	if (!sys::fs::dirExists(path)) {
		return;
	}

	PDEBUG << "Clearing cache in dir: " << path << endl;
	std::vector<std::string> files = sys::fs::ls(path);
	for (size_t i = 0; i < files.size(); i++) {
		std::string fullpath = path + "/" + files[i];
		PDEBUG << "Unlinking " << fullpath << endl;
		sys::fs::unlink(fullpath);
	}
}

// Checks the cache version
Cache::VersionCheckResult Cache::checkVersion(int version)
{
	if (version <= 0) {
		return UnknownVersion;
	}
	if (version <= 1) {
		// The diffstats for Mercurial and Git have been flawed in version 1.
		// The Subversion backend uses repository-wide diffstats now.
		goto outofdate;
	}
	if (version <= 2 && m_backend->name() == "subversion") {
		// Invalid diffstats for deleted files in version 2 (Subversion backend)
		goto outofdate;
	}
	if (version <= 3 && m_backend->name() == "git") {
		// Invalid commit times in version 3 (Git backend)
		goto outofdate;
	}
	if ((uint32_t)version <= CACHE_VERSION) {
		return Ok;
	}

	return UnknownVersion;

outofdate:
	Logger::warn() << "Warning: Cache is out of date, clearing" << endl;
	return Clear;
}

// Ensures that the cache dir is writable and exists
void Cache::checkDir(const std::string &path, bool *created)
{
	if (!sys::fs::dirExists(path)) {
		// Create the cache directory
		try {
			sys::fs::mkpath(path);
		} catch (const std::exception &ex) {
			throw PEX(utils::strprintf("Unable to create cache directory: %s", ex.what()));
		}
		PDEBUG << "Cache: Creating cache directory '" << path << '\'' << endl;

		if (created) {
			*created = true;
		}
	} else if (created) {
		*created = false;
	}
}

// Checks cache entries and removes invalid ones from the index file
void Cache::check()
{
	std::map<std::string, std::pair<uint32_t, uint32_t> > index;

	std::string path = m_opts.cacheDir() + "/" + uuid();
	PDEBUG << "Checking cache in dir: " << path << endl;

	bool created;
	checkDir(path, &created);
	if (created) {
		Logger::info() << "Cache: Created empty cache for '" << uuid() << '\'' << endl;
		return;
	}
	sys::datetime::Watch watch;

	GZIStream *in = new GZIStream(path+"/index");
	if (!in->ok()) {
		Logger::info() << "Cache: Empty cache for '" << uuid() << '\'' << endl;
		delete in;
		return;
	}

	BIStream *cache_in = NULL;
	uint32_t cache_index = -1;

	uint32_t version;
	*in >> version;
	if (version == 0 || version > CACHE_VERSION) {
		Logger::warn() << "Cache: Unkown cache version number " << version << ", clearing" << endl;
		delete in;
		clear();
		return;
	}

	switch (checkVersion(version)) {
		case Abort:
			delete in;
			return;
		case Clear:
			delete in;
			clear();
			return;
		default:
			break;
	}

	Logger::status() << "Checking all indexed revisions... " << ::flush;

	std::string id;
	std::pair<uint32_t, uint32_t> pos;
	uint32_t crc;
	std::map<std::string, uint32_t> crcs;
	std::vector<std::string> corrupted;
	while (!in->eof()) {
		*in >> id;
		if (id.empty()) {
			break;
		}
		*in >> pos.first >> pos.second;
		*in >> crc;

		index[id] = pos;
		crcs[id] = crc;

		bool ok = true;
		if (cache_in == NULL || cache_index != pos.first) {
			delete cache_in;
			cache_in = new BIStream(utils::strprintf("%s/cache.%u", path.c_str(), pos.first));
			cache_index = pos.first;
			if (!cache_in->ok()) {
				delete cache_in; cache_in = NULL;
				ok = false;
			}
		}
		if (cache_in != NULL) {
			if (!cache_in->seek(pos.second)) {
				ok = false;
			} else {
				std::vector<char> data;
				*cache_in >> data;
				if (utils::crc32(data) != crc) {
					ok = false;
				}
			}
		}

		if (!ok) {
			PTRACE << "Revision " << id << " corrupted!" << endl;
			std::cerr << "Cache: Revision " << id << " is corrupted, removing from index file" << std::endl;
			corrupted.push_back(id);
		} else {
			PTRACE << "Revision " << id << " ok" << endl;
		}
	}
	delete cache_in;
	delete in;

	Logger::status() << "done" << endl;

	Logger::info() << "Cache: Checked " << index.size() << " revisions in " << watch.elapsedMSecs() << " ms" << endl;

	if (corrupted.empty()) {
		Logger::info() << "Cache: Everything's alright" << endl;
		return;
	}

	Logger::info() << "Cache: " << corrupted.size() << " corrupted revisions, rewriting index file" << endl;

	// Remove corrupted revisions from index file
	for (unsigned int i = 0; i < corrupted.size(); i++) {
		std::map<std::string, std::pair<uint32_t, uint32_t> >::iterator it = index.find(corrupted[i]);
		if (it != index.end()) {
			index.erase(it);
		}
	}

	// Defer any signals while writing to the cache
	{
		SIGBLOCK_DEFER();

		// Rewrite index file
		GZOStream out(path+"/index");
		out << CACHE_VERSION;
		for (std::map<std::string, std::pair<uint32_t, uint32_t> >::iterator it = index.begin(); it != index.end(); ++it) {
			out << it->first;
			out << it->second.first << it->second.second;
			out << crcs[it->first];
		}
	}
}
