/*
 * pepper - SCM statistics report generator
 * Copyright (C) 2010-2014 Jonas Gehring
 *
 * Released under the GNU General Public License, version 3.
 * Please see the COPYING file in the source distribution for license
 * terms and conditions, or see http://www.gnu.org/licenses/.
 *
 * file: ldbcache.h
 * Revision cache using Leveldb (interface)
 */


#ifndef LDBCACHE_H_
#define LDBCACHE_H_


#include "abstractcache.h"

class Cache;

namespace leveldb {
	class DB;
}


class LdbCache : public AbstractCache
{
	public:
		LdbCache(Backend *backend, const Options &options);
		~LdbCache();

		void flush();
		void check(bool force = false);

	protected:
		bool lookup(const std::string &id);
		void put(const std::string &id, const Revision &rev);
		Revision *get(const std::string &id);

	private:
		void opendb();
		void closedb();
		void import(Cache *cache);

	private:
		leveldb::DB *m_db;
};


#endif // CACHE_H_
