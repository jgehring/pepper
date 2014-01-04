/*
 * pepper - SCM statistics report generator
 * Copyright (C) 2010-2014 Jonas Gehring
 *
 * Released under the GNU General Public License, version 3.
 * Please see the COPYING file in the source distribution for license
 * terms and conditions, or see http://www.gnu.org/licenses/.
 *
 * file: cache.h
 * Revision cache with custom binary format (interface)
 */


#ifndef CACHE_H_
#define CACHE_H_


#include "abstractcache.h"

class BIStream;
class BOStream;


class Cache : public AbstractCache
{
	friend class LdbCache; // For importing revisions

	private:
		typedef enum {
			Ok,
			Abort,
			UnknownVersion,
			OutOfDate
		} VersionCheckResult;

	public:
		Cache(Backend *backend, const Options &options);
		~Cache();

		void flush();
		void check(bool force = false);

	protected:
		bool lookup(const std::string &id);
		void put(const std::string &id, const Revision &rev);
		Revision *get(const std::string &id);

	private:
		void load();
		void clear();
		void lock();
		void unlock();
		VersionCheckResult checkVersion(int version);

	private:
		BOStream *m_iout, *m_cout;
		BIStream *m_cin;
		uint32_t m_coindex, m_ciindex;
		bool m_loaded;
		int m_lock;

		std::map<std::string, std::pair<uint32_t, uint32_t> > m_index;
};


#endif // CACHE_H_
