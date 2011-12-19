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
#include "logger.h"
#include "revision.h"
#include "strlib.h"
#include "utils.h"

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
void LdbCache::check(bool force)
{

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
	std::vector<char> compressed = utils::compress(rout.data());
	leveldb::Status s = m_db->Put(leveldb::WriteOptions(), id, std::string(compressed.begin(), compressed.end()));
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
	std::vector<char> data = utils::uncompress(std::vector<char>(value.begin(), value.end()));
	MIStream rin(data);
	if (!rev->load(rin)) {
		throw PEX(str::printf("Unable to read from cache: Data corrupted"));
	}
	return rev;
}

void LdbCache::opendb()
{
	if (m_db) return;

	std::string path = cacheDir() + "/ldb";
	leveldb::Options options;
	options.create_if_missing = true;
	leveldb::Status s = leveldb::DB::Open(options, path, &m_db);
	if (!s.ok()) {
		throw PEX(str::printf("Unable to open database %s: %s", path.c_str(), s.ToString().c_str()));
	}
}

void LdbCache::closedb()
{
	delete m_db;
	m_db = NULL;
}
