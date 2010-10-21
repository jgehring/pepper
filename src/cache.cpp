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


// Constructor
Cache::Cache(Backend *backend, const Options &options)
	: Backend(options), m_backend(backend)
{
	load();
}

// Destructor
Cache::~Cache()
{
	delete m_backend;
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
	uint32_t index = 0, offset = 0;
	do {
		// TODO: Avoid stats, i.e. keep stream open
		path = Utils::strprintf("%s/cache.%u", dir.c_str(), index);
		if (SysUtils::filesize(path) < 4194304/16) {
			break;
		}
		++index;
	} while (true);

	BOStream out(path, true);
	offset = out.tell();
	stat.write(out);

	// Add revision to index
	BOStream iout(dir + "/index", true);
	if (iout.tell() == 0) {
		// Version number
		iout << (uint32_t)1;
	}
	iout << id;
	iout << index << offset;
}

// Loads a diffstat from the cache
Diffstat Cache::get(const std::string &id)
{
	std::string dir = m_opts.cacheDir() + "/" + m_backend->uuid();
	std::pair<uint32_t, uint32_t> offset = m_index[id];
	std::string path = Utils::strprintf("%s/cache.%u", dir.c_str(), offset.first);
	BIStream in(path);
	if (!in.ok()) {
		throw Utils::strprintf("Unable to read from cache file: %s", path.c_str());
	}
	if (!in.seek(offset.second)) {
		throw Utils::strprintf("Unable to read from cache file: %s", path.c_str());
	}

	Diffstat stat;
	if (!stat.load(in)) {
		throw Utils::strprintf("Unable to read from cache file: %s", path.c_str());
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
			throw Utils::strprintf("Unable to create cache directory: %s", path.c_str());
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
		throw Utils::strprintf("Unkown cache version number %u", version);
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
