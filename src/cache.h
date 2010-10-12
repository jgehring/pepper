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

		void init() { m_backend->init(); }

		std::string name() const { return m_backend->name(); }
		std::string head(const std::string &branch = std::string()) { return m_backend->head(branch); }
		std::vector<std::string> branches() { return m_backend->branches(); }

		Revision *revision(const std::string &id);

	private:
		Backend *m_backend;
};


#endif // CACHE_H_
