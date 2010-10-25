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

#include "bstream.h"
#include "diffstat.h"
#include "options.h"
#include "revision.h"
#include "sysutils.h"
#include "utils.h"

#include "cache.h"


#define MAX_CACHEFILE_SIZE 4194304


// Constructor
Cache::Cache(Backend *backend, const Options &options)
	: Backend(options), m_backend(backend), m_iout(NULL), m_cout(NULL),
	  m_cin(0), m_coindex(0), m_ciindex(0)
{
	load();
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
		Diffstat stat = m_backend->diffstat(id);
		put(id, stat);
		return stat;
	}

	return get(id);
}

// Returns the revision data for the given ID
Revision *Cache::revision(const std::string &id)
{
	// TODO: Add meta data
	return new Revision(id, diffstat(id));
}

// Checks if the diffstat of the given revision is already cached
bool Cache::lookup(const std::string &id)
{
	return (m_index.find(id) != m_index.end());
}

// Adds the diffstat of the given revision to the cache
void Cache::put(const std::string &id, const Diffstat &stat)
{
	// Add revision to cache
	std::string dir = m_opts.cacheDir() + "/" + m_backend->uuid(), path;
	if (m_cout == NULL) {
		m_coindex = 0;
		do {
			path = Utils::strprintf("%s/cache.%u", dir.c_str(), m_coindex);
			// TODO: For compressed files, the stat returns the actual data size
			if (SysUtils::filesize(path) < MAX_CACHEFILE_SIZE) {
				break;
			}
			++m_coindex;
		} while (true);

		delete m_cout; m_cout = NULL;
		m_cout = new BOStream(path, true);
	} else if (m_cout->tell() >= MAX_CACHEFILE_SIZE) {
		delete m_cout;
		path = Utils::strprintf("%s/cache.%u", dir.c_str(), ++m_coindex);
		m_cout = new BOStream(path, true);
	}

	uint32_t offset = m_cout->tell();
	stat.write(*m_cout);

	// Add revision to index
	if (m_iout == NULL) {
		if (SysUtils::exists(dir + "/index")) {
			m_iout = new BOStream(dir + "/index", true);
		} else {
			m_iout = new BOStream(dir + "/index", false);
			// Version number
			*m_iout << (uint32_t)1;
		}
	}
	*m_iout << id;
	*m_iout << m_coindex << offset;
}

// Loads a diffstat from the cache
Diffstat Cache::get(const std::string &id)
{
	std::string dir = m_opts.cacheDir() + "/" + m_backend->uuid();
	std::pair<uint32_t, uint32_t> offset = m_index[id];
	std::string path = Utils::strprintf("%s/cache.%u", dir.c_str(), offset.first);
	if (m_cin == NULL || offset.first != m_ciindex) {
		delete m_cout; m_cout = NULL;
		m_cin = new BIStream(path);
		m_ciindex = offset.first;
		if (!m_cin->ok()) {
			throw PEX(Utils::strprintf("Unable to read from cache file: %s", path.c_str()));
		}
	}
	if (!m_cin->seek(offset.second)) {
		throw PEX(Utils::strprintf("Unable to read from cache file: %s", path.c_str()));
	}

	Diffstat stat;
	if (!stat.load(*m_cin)) {
		throw PEX(Utils::strprintf("Unable to read from cache file: %s", path.c_str()));
	}
	return stat;
}

// Loads the index file of the cache
void Cache::load()
{
	m_index.clear();

	std::string path = m_opts.cacheDir() + "/" + m_backend->uuid();
	struct stat statbuf;
	if (stat(path.c_str(), &statbuf) == -1) {
		// Create the cache directory
		if (SysUtils::mkpath(path) < 0) {
			throw PEX(Utils::strprintf("Unable to create cache directory: %s", path.c_str()));
		}
		return;
	}

	BIStream in(path+"/index");
	if (!in.ok()) {
		return;
	}

	uint32_t version;
	in >> version;
	if (version != 1) {
		throw PEX(Utils::strprintf("Unkown cache version number %u", version));
	}

	std::string buffer;
	std::pair<uint32_t, uint32_t> pos;
	while (!in.eof()) {
		in >> buffer;
		if (buffer.empty()) {
			return;
		}
		in >> pos.first >> pos.second;
		m_index[buffer] = pos;
	}
}
