/*
 * pepper - SCM statistics report generator
 * Copyright (C) 2010 Jonas Gehring
 *
 * file: cache.cpp
 * Revision cache
 */


#include <iostream>
#include <fstream>
#include <cstdlib>
#include <cstring>

#include <sys/stat.h>
#include <sys/time.h>

#include "main.h"

#include "bstream.h"
#include "diffstat.h"
#include "fs.h"
#include "globals.h"
#include "logger.h"
#include "options.h"
#include "revision.h"
#include "utils.h"

#include "cache.h"


#define CACHE_VERSION (uint32_t)1
#define MAX_CACHEFILE_SIZE 4194304


// Constructor
Cache::Cache(Backend *backend, const Options &options)
	: Backend(options), m_backend(backend), m_iout(NULL), m_cout(NULL),
	  m_cin(0), m_coindex(0), m_ciindex(0)
{

}

// Destructor
Cache::~Cache()
{
	delete m_backend;
	delete m_iout;
	delete m_cout;
	delete m_cin;
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

// Flushes and closes the cache streams
void Cache::flush()
{
	delete m_iout;
	m_iout = NULL;
	delete m_cout;
	m_cout = NULL;
	delete m_cin;
	m_cin = NULL;
}

// Checks if the diffstat of the given revision is already cached
bool Cache::lookup(const std::string &id)
{
	return (m_index.find(id) != m_index.end());
}

// Adds the revision of the given revision to the cache
void Cache::put(const std::string &id, const Revision &rev)
{
	sys::parallel::MutexLocker locker(&Globals::cacheMutex);

	// Add revision to cache
	std::string dir = m_opts.cacheDir() + "/" + uuid(), path;
	if (m_cout == NULL) {
		m_coindex = 0;
		do {
			path = utils::strprintf("%s/cache.%u", dir.c_str(), m_coindex);
			// TODO: For compressed files, the stat returns the actual data size
			if (sys::fs::filesize(path) < MAX_CACHEFILE_SIZE) {
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

	std::string path = m_opts.cacheDir() + "/" + uuid();
	PDEBUG << "Using cache dir: " << path << endl;
	struct stat statbuf;
	if (stat(path.c_str(), &statbuf) == -1) {
		// Create the cache directory
		if (sys::fs::mkpath(path) < 0) {
			throw PEX(utils::strprintf("Unable to create cache directory: %s", path.c_str()));
		}
		Logger::info() << "Cache: Creating cache directory for '" << uuid() << '\'' << endl;
		return;
	}

	// For git repositories, the hardest part is calling uuid()
	timeval tv;
	gettimeofday(&tv, NULL);

	GZIStream in(path+"/index");
	if (!in.ok()) {
		Logger::info() << "Cache: Empty cache for '" << uuid() << '\'' << endl;
		return;
	}

	uint32_t version;
	in >> version;
	if (version != CACHE_VERSION) {
		throw PEX(utils::strprintf("Unkown cache version number %u", version));
	}

	Logger::status() << "Loading cache index... " << ::flush;

	std::string buffer;
	std::pair<uint32_t, uint32_t> pos;
	uint32_t crc;
	while (!in.eof()) {
		in >> buffer;
		if (buffer.empty()) {
			break;
		}
		in >> pos.first >> pos.second;
		in >> crc;
		m_index[buffer] = pos;
	}

	Logger::status() << "done" << endl;

	timeval c;
	gettimeofday(&c, NULL);	
 	Logger::info() << "Cache: Loaded " << m_index.size() << " revisions in " << (c.tv_sec - tv.tv_sec) * 1000 + (c.tv_usec - tv.tv_usec) / 1000 << " ms" << endl;
}

// Checks cache entries and removes invalid ones from the index file
void Cache::check()
{
	std::map<std::string, std::pair<uint32_t, uint32_t> > index;

	std::string path = m_opts.cacheDir() + "/" + uuid();
	PDEBUG << "Checking cache in dir: " << path << endl;
	struct stat statbuf;
	if (stat(path.c_str(), &statbuf) == -1) {
		// Create the cache directory
		if (sys::fs::mkpath(path) < 0) {
			throw PEX(utils::strprintf("Unable to create cache directory: %s", path.c_str()));
		}
		Logger::info() << "Cache: Creating cache directory for '" << uuid() << '\'' << endl;
		return;
	}

	// TODO: Add timing framework
	timeval tv;
	gettimeofday(&tv, NULL);

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
	if (version != CACHE_VERSION) {
		throw PEX(utils::strprintf("Unkown cache version number %u", version));
	}

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

	timeval c;
	gettimeofday(&c, NULL);	
 	Logger::info() << "Cache: Checked " << index.size() << " revisions in " << (c.tv_sec - tv.tv_sec) * 1000 + (c.tv_usec - tv.tv_usec) / 1000 << " ms" << endl;

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

	// Rewrite index file
	GZOStream out(path+"/index");
	out << CACHE_VERSION;
	for (std::map<std::string, std::pair<uint32_t, uint32_t> >::iterator it = index.begin(); it != index.end(); ++it) {
		out << it->first;
		out << it->second.first << it->second.second;
		out << crcs[it->first];
	}
}
