/*
 * pepper - SCM statistics report generator
 * Copyright (C) 2010 Jonas Gehring
 *
 * file: cache.cpp
 * Revision cache
 */


#include "cache.h"


// Constructor
Cache::Cache(Backend *backend, const Options &options)
	: Backend(options), m_backend(backend)
{

}

// Destructor
Cache::~Cache()
{

}

// Returns the revision data for the given ID
Revision *Cache::revision(const std::string &id)
{
	// TODO
	return m_backend->revision(id);
}
