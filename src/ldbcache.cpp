/*
 * pepper - SCM statistics report generator
 * Copyright (C) 2010-2011 Jonas Gehring
 *
 * Released under the GNU General Public License, version 3.
 * Please see the COPYING file in the source distribution for license
 * terms and conditions, or see http://www.gnu.org/licenses/.
 *
 * file: ldbcache.cpp
 * Revision cache using Leveldb
 */


#include "main.h"

#include <leveldb/db.h>

#include "bstream.h"
#include "cache.h"
#include "logger.h"
#include "revision.h"
#include "strlib.h"
#include "utils.h"

#include "syslib/fs.h"

#include "ldbcache.h"


// Constructor
LdbCache::LdbCache(Backend *backend, const Options &options)
	: AbstractCache(backend, options), m_db(NULL)
{

}

// Destructor
LdbCache::~LdbCache()
{
	closedb();
}

// Flushes the cache to disk
void LdbCache::flush()
{

}

// Checks cache consistency
void LdbCache::check(bool /* force */)
{
	try {
		if (!m_db) opendb();
		Logger::info() << "LdbCache: Database opened, everything's alright" << endl;
	} catch (const std::exception &ex) {
		PDEBUG << "Exception while opening database: " << ex.what() << endl;
		PDEBUG << "Trying to repair it" << endl;

		Logger::info() << "LdbCache: Database can't be opened, trying to repair it" << endl;
		std::string path = cacheDir() + "/ldb";
		leveldb::Status status = leveldb::RepairDB(path, leveldb::Options());
		if (!status.ok()) {
			Logger::err() << "Error repairing database: " << status.ToString() << endl;
		} else {
			Logger::info() << "LdbCache: Database repaired, please re-run to check revisions" << endl;
		}
		return;
	}

	// Simply try to read all revisions
	std::vector<std::string> corrupted;
	size_t n = 0;
	leveldb::Iterator* it = m_db->NewIterator(leveldb::ReadOptions());
	for (it->SeekToFirst(); it->Valid(); it->Next()) {
		Revision rev(it->key().ToString());
		std::string value = it->value().ToString();
		MIStream rin(value.c_str(), value.length());
		if (!rev.load(rin)) {
			PDEBUG << "Revision " << it->key().ToString() << " corrupted!" << endl;
			corrupted.push_back(it->key().ToString());
		}
		++n;
	}
	if (!it->status().ok()) {
		Logger::err() << "Error iterating over cached revisions: " << it->status().ToString() << endl;
		return;
	}
	Logger::info() << "LdbCache: Checked " << n << " revisions, found " << corrupted.size() << " to be corrupted" << endl;

	for (size_t i = 0; i < corrupted.size(); i++) {
		Logger::err() << "LdbCache: Revision " << corrupted[i] << " is corrupted, removing from index file" << endl;
		leveldb::Status status = m_db->Delete(leveldb::WriteOptions(), corrupted[i]);
		if (!status.ok()) {
			Logger::err() << "Error: Can't remove from revision " << corrupted[i] << " from database: " << status.ToString() << endl;
			return;
		}
	}
}

// Checks if the diffstat of the given revision is already cached
bool LdbCache::lookup(const std::string &id)
{
	if (!m_db) opendb();

	std::string value;
	leveldb::Status s = m_db->Get(leveldb::ReadOptions(), id, &value);
	if (s.IsNotFound()) {
		return false;
	}
	if (!s.ok()) {
		throw PEX(str::printf("Error reading from cache: %s", s.ToString().c_str()));
	}
	return true;
}

// Adds the revision to the cache
void LdbCache::put(const std::string &id, const Revision &rev)
{
	if (!m_db) opendb();

	MOStream rout;
	rev.write(rout);
	std::vector<char> data(rout.data());
	leveldb::Status s = m_db->Put(leveldb::WriteOptions(), id, std::string(data.begin(), data.end()));
	if (!s.ok()) {
		throw PEX(str::printf("Error writing to cache: %s", s.ToString().c_str()));
	}
}

Revision *LdbCache::get(const std::string &id)
{
	if (!m_db) opendb();

	std::string value;
	leveldb::Status s = m_db->Get(leveldb::ReadOptions(), id, &value);
	if (!s.ok()) {
		throw PEX(str::printf("Error reading from cache: %s", s.ToString().c_str()));
	}

	Revision *rev = new Revision(id);
	MIStream rin(value.c_str(), value.length());
	if (!rev->load(rin)) {
		throw PEX(str::printf("Unable to read from cache: Data corrupted"));
	}
	return rev;
}

// Opens the database connection
void LdbCache::opendb()
{
	if (m_db) return;

	std::string path = cacheDir() + "/ldb";
	PDEBUG << "Using cache dir: " << path << endl;
	if (!sys::fs::dirExists(path)) {
		sys::fs::mkpath(path);
	}
	leveldb::Options options;
	options.create_if_missing = false;
	leveldb::Status s = leveldb::DB::Open(options, path, &m_db);
	if (!s.ok()) {
		// New cache: Import revisions from old cache
		options.create_if_missing = true;
		leveldb::Status s = leveldb::DB::Open(options, path, &m_db);
		if (!s.ok()) {
			throw PEX(str::printf("Unable to open database %s: %s", path.c_str(), s.ToString().c_str()));
		}

		Cache c(m_backend, m_opts);
		import(&c);
	}
}

// Closes the database connection
void LdbCache::closedb()
{
	delete m_db;
	m_db = NULL;
}

// Imports all revisions from the given cache
void LdbCache::import(Cache *cache)
{
	try {
		cache->load();
	} catch (const std::exception &ex) {
		PDEBUG << "Error loading old cache for import: " << ex.what() << endl;
		return;
	}

	Logger::info() << "LdbCache: Found old cache, importing revisions..." << endl;
	std::map<std::string, std::pair<uint32_t, uint32_t> >::const_iterator it;
	size_t n = 0;
	for (it = cache->m_index.begin(); it != cache->m_index.end(); ++it) {
		Revision *rev = cache->get(it->first);
		put(it->first, *rev);
		delete rev;
		++n;
	}
	Logger::info() << "LdbCache: Imported " << n << " revisions" << endl;
}
