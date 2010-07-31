/*
 * pepper - SCM statistics report generator
 * Copyright (C) 2010 Jonas Gehring
 *
 * file: cache.h
 * Revision cache (interface)
 */


#ifndef CACHE_H_
#define CACHE_H_


#include "backend.h"

class Revision;


// This cache should be transparent and inherits the wrapped class
class Cache : public Backend
{
	public:
		Cache(Backend *backend, const Options &options);
		~Cache();

		std::string name() const { return m_backend->name(); }

		Revision *revision(const std::string &id);

	private:
		Backend *m_backend;
};


#endif // CACHE_H_
