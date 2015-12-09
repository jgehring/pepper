/*
 * pepper - SCM statistics report generator
 * Copyright (C) 2010-2014 Jonas Gehring
 *
 * Released under the GNU General Public License, version 3.
 * Please see the COPYING file in the source distribution for license
 * terms and conditions, or see http://www.gnu.org/licenses/.
 *
 * file: abstractcache.h
 * Abstract base class for revision caches (interface)
 */


#ifndef ABSTRACTCACHE_H_
#define ABSTRACTCACHE_H_


#include "backend.h"

class Revision;


// This cache should be transparent and inherits the wrapped class
class AbstractCache : public Backend
{
	public:
		AbstractCache(Backend *backend, const Options &options);
		virtual ~AbstractCache();

		void init() { }
		void open() { m_backend->open(); }
		void close() { flush(); m_backend->close(); }

		std::string name() const { return m_backend->name(); }
		std::string uuid() { if (m_uuid.empty()) m_uuid = m_backend->uuid(); return m_uuid; }

		std::string head(const std::string &branch = std::string()) { return m_backend->head(branch); }
		std::string mainBranch() { return m_backend->mainBranch(); }
		std::vector<std::string> branches() { return m_backend->branches(); }
		std::vector<Tag> tags() { return m_backend->tags(); }
		DiffstatPtr diffstat(const std::string &id);
		void filterDiffstat(DiffstatPtr stat) { m_backend->filterDiffstat(stat); }
		std::vector<std::string> tree(const std::string &id = std::string()) { return m_backend->tree(id); }
		std::string cat(const std::string &path, const std::string &id = std::string()) { return m_backend->cat(path, id); };

		LogIterator *iterator(const std::string &branch = std::string(), int64_t start = -1, int64_t end = -1) { return m_backend->iterator(branch, start, end); }
		void prefetch(const std::vector<std::string> &ids);
		Revision *revision(const std::string &id);
		void finalize() { m_backend->finalize(); }

		static std::string cacheFile(Backend *backend, const std::string &name);

		virtual void flush() = 0;
		virtual void check(bool force = false) = 0;

	protected:
		std::string cacheDir();

		virtual bool lookup(const std::string &id) = 0;
		virtual void put(const std::string &id, const Revision &rev) = 0;
		virtual Revision *get(const std::string &id) = 0;

		static void checkDir(const std::string &path, bool *created = NULL);

	protected:
		Backend *m_backend;
		std::string m_uuid; // Cached backend UUID
};


#endif // ABSTRACTCACHE_H_
