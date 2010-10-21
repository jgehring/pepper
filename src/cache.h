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

class BIStream;
class BOStream;
class Revision;


// This cache should be transparent and inherits the wrapped class
class Cache : public Backend
{
	public:
		Cache(Backend *backend, const Options &options);
		~Cache();

		void init() { m_backend->init(); }

		std::string name() const { return m_backend->name(); }

		std::string uuid() { return m_backend->uuid(); }

		std::string head(const std::string &branch = std::string()) { return m_backend->head(branch); }
		std::vector<std::string> branches() { return m_backend->branches(); }
		Diffstat diffstat(const std::string &id);

		RevisionIterator *iterator(const std::string &branch = std::string()) { return m_backend->iterator(branch); }
		Revision *revision(const std::string &id);

	private:
		bool lookup(const std::string &id);
		void put(const std::string &id, const Diffstat &stat);
		Diffstat get(const std::string &id);
		void load();

	private:
		Backend *m_backend;
		BOStream *m_iout, *m_cout;
		BIStream *m_cin;
		uint32_t m_coindex, m_ciindex;

		std::map<std::string, std::pair<uint32_t, uint32_t> > m_index;
};


#endif // CACHE_H_
