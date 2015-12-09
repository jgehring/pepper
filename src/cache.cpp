/*
 * pepper - SCM statistics report generator
 * Copyright (C) 2010-present Jonas Gehring
 *
 * Released under the GNU General Public License, version 3.
 * Please see the COPYING file in the source distribution for license
 * terms and conditions, or see http://www.gnu.org/licenses/.
 *
 * file: cache.cpp
 * Revision cache with custom binary format
 */


#include "main.h"

#include <cstring>
#include <fcntl.h>
#include <unistd.h>

#include "bstream.h"
#include "logger.h"
#include "options.h"
#include "revision.h"
#include "strlib.h"
#include "utils.h"

#include "syslib/datetime.h"
#include "syslib/fs.h"
#include "syslib/sigblock.h"

#include "cache.h"

#define CACHE_VERSION (uint32_t)5
#define MAX_CACHEFILE_SIZE 4194304


// Constructor
Cache::Cache(Backend *backend, const Options &options)
	: AbstractCache(backend, options), m_iout(NULL), m_cout(NULL),
	  m_cin(0), m_coindex(0), m_ciindex(0), m_loaded(false), m_lock(-1)
{

}

// Destructor
Cache::~Cache()
{
	flush();
	unlock();
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

// Adds the revision to the cache
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
			path = str::printf("%s/cache.%u", dir.c_str(), m_coindex);
			if (!sys::fs::fileExists(path) || sys::fs::filesize(path) < MAX_CACHEFILE_SIZE) {
				break;
			}
			++m_coindex;
		} while (true);

		delete m_cout;
		m_cout = new BOStream(path, true);
	} else if (m_cout->tell() >= MAX_CACHEFILE_SIZE) {
		delete m_cout;
		path = str::printf("%s/cache.%u", dir.c_str(), ++m_coindex);
		m_cout = new BOStream(path, true);
	}

	uint32_t offset = m_cout->tell();
	MOStream rout;
	rev.write03(rout);
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

	// Update cached index
	m_index[id] = std::pair<uint32_t, uint32_t>(m_coindex, offset);
}

// Loads a revision from the cache
Revision *Cache::get(const std::string &id)
{
	if (!m_loaded) {
		load();
	}

	std::string dir = cacheDir();
	std::pair<uint32_t, uint32_t> offset = m_index[id];
	std::string path = str::printf("%s/cache.%u", dir.c_str(), offset.first);
	if (m_cin == NULL || offset.first != m_ciindex) {
		delete m_cin;
		m_cin = new BIStream(path);
		m_ciindex = offset.first;
		if (!m_cin->ok()) {
			throw PEX(str::printf("Unable to read from cache file: %s", path.c_str()));
		}
	}
	if (!m_cin->seek(offset.second)) {
		throw PEX(str::printf("Unable to read from cache file: %s", path.c_str()));
	}

	Revision *rev = new Revision(id);
	std::vector<char> data;
	*m_cin >> data;
	data = utils::uncompress(data);
	if (data.empty()) {
		throw PEX(str::printf("Unable to read from cache file: %s", path.c_str()));
	}
	MIStream rin(data);
	if (!rev->load03(rin)) {
		throw PEX(str::printf("Unable to read from cache file: %s", path.c_str()));
	}
	return rev;
}

// Loads the index file
void Cache::load()
{
	std::string path = cacheDir();
	PDEBUG << "Using cache dir: " << path << endl;

	m_index.clear();
	m_loaded = true;

	bool created;
	checkDir(path, &created);
	lock();
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
		case OutOfDate:
			delete in;
			throw PEX(str::printf("Cache is out of date - please run the check_cache report", version));
		case UnknownVersion:
			delete in;
			throw PEX(str::printf("Unknown cache version number %u - please run the check_cache report", version));
		default:
			break;
	}

	Logger::status() << "Loading cache index... " << ::flush;

	std::string buffer;
	std::pair<uint32_t, uint32_t> pos;
	uint32_t crc;
	while (!(*in >> buffer).eof()) {
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

	std::string path = cacheDir();
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

// Locks the cache directory for this process
void Cache::lock()
{
	if (m_lock >= 0) {
		PDEBUG << "Already locked (" << m_lock << ")" << endl;
		return;
	}

	std::string path = cacheDir();
	std::string lock = path + "/lock";
	m_lock = ::open(lock.c_str(), O_WRONLY | O_CREAT, S_IWUSR);
	if (m_lock == -1) {
		throw PEX(str::printf("Unable to lock cache %s: %s", path.c_str(), PepperException::strerror(errno).c_str()));
	}

	PTRACE << "Locking file " << lock << endl;
	struct flock flck;
	memset(&flck, 0x00, sizeof(struct flock));
	flck.l_type = F_WRLCK;
	if (fcntl(m_lock, F_SETLK, &flck) == -1) {
		throw PEX(str::printf("Unable to lock cache %s, it may be used by another instance", path.c_str()));
	}
}

// Unlocks the cache directory
void Cache::unlock()
{
	if (m_lock < 0) {
		PDEBUG << "Not locked yet (" << m_lock << ")" << endl;
		return;
	}

	std::string path = cacheDir();
	PTRACE << "Unlocking file " << path + "/lock" << endl;
	struct flock flck;
	memset(&flck, 0x00, sizeof(struct flock));
	flck.l_type = F_UNLCK;
	if (fcntl(m_lock, F_SETLK, &flck) == -1) {
		throw PEX(str::printf("Unable to unlock cache, please delete %s/lock manually if required", path.c_str()));
	}
	if (::close(m_lock) == -1) {
		throw PEX_ERRNO();
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
		return OutOfDate;
	}
	if (version <= 2 && m_backend->name() == "subversion") {
		// Invalid diffstats for deleted files in version 2 (Subversion backend)
		return OutOfDate;
	}
	if (version <= 4 && m_backend->name() == "git") {
		// Invalid commit times in version 3 (Git backend)
		return OutOfDate;
	}
	if ((uint32_t)version <= CACHE_VERSION) {
		return Ok;
	}

	return UnknownVersion;
}

// Checks cache entries and removes invalid ones from the index file
void Cache::check(bool force)
{
	std::map<std::string, std::pair<uint32_t, uint32_t> > index;

	std::string path = cacheDir();
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
	switch (checkVersion(version)) {
		case OutOfDate:
			delete in;
			Logger::warn() << "Cache: Cache is out of date";
			if (!force) {
				Logger::warn() << " - won't clear it until forced to do so" << endl;
			} else {
				Logger::warn() << ", clearing" << endl;
				clear();
			}
			return;
		case UnknownVersion:
			delete in;
			Logger::warn() << "Cache: Unknown cache version number " << version;
			if (!force) {
				Logger::warn() << " - won't clear it until forced to do so" << endl;
			} else {
				Logger::warn() << ", clearing" << endl;
				clear();
			}
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
	while (!(*in >> id).eof()) {
		if (!in->ok()) {
			goto corrupt;
		}

		*in >> pos.first >> pos.second;
		*in >> crc;
		if (!in->ok()) {
			goto corrupt;
		}

		index[id] = pos;
		crcs[id] = crc;

		if (cache_in == NULL || cache_index != pos.first) {
			delete cache_in;
			cache_in = new BIStream(str::printf("%s/cache.%u", path.c_str(), pos.first));
			cache_index = pos.first;
			if (!cache_in->ok()) {
				delete cache_in; cache_in = NULL;
				goto corrupt;
			}
		}
		if (cache_in != NULL) {
			if (!cache_in->seek(pos.second)) {
				goto corrupt;
			} else {
				std::vector<char> data;
				*cache_in >> data;
				if (utils::crc32(data) != crc) {
					goto corrupt;
				}
			}
		}

		PTRACE << "Revision " << id << " ok" << endl;
		continue;

corrupt:
		PTRACE << "Revision " << id << " corrupted!" << endl;
		std::cerr << "Cache: Revision " << id << " is corrupted, removing from index file" << std::endl;
		corrupted.push_back(id);
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
