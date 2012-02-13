/*
 * pepper - SCM statistics report generator
 * Copyright (C) 2010-2012 Jonas Gehring
 *
 * Released under the GNU General Public License, version 3.
 * Please see the COPYING file in the source distribution for license
 * terms and conditions, or see http://www.gnu.org/licenses/.
 *
 * file: subversion_p.h
 * Interfaces of internal classes for the Subversion backend
 */


#ifndef SUBVERSION_BACKEND_P_H_
#define SUBVERSION_BACKEND_P_H_


#include <apr_pools.h>
#include <svn_client.h>
#include <svn_ra.h>

#include "diffstat.h"

template <typename Arg, typename Result> class JobQueue;


// Repository connection
class SvnConnection
{
	public:
		SvnConnection();
		~SvnConnection();

		void open(const std::string &url, const std::map<std::string, std::string> &options);
		void open(SvnConnection *parent);
		static std::string strerr(svn_error_t *err);
		
	private:
		void init();

		template <typename T>
		static T hashget(apr_hash_t *hash, const char *key)
		{
			return static_cast<T>(apr_hash_get(hash, key, APR_HASH_KEY_STRING));
		}

	public:
		apr_pool_t *pool;
		svn_client_ctx_t *ctx;
		svn_ra_session_t *ra;
		const char *url, *root, *prefix;
};


// The diffstat thread is implemented in subversion_delta.cpp
class SvnDiffstatThread : public sys::parallel::Thread
{
	public:
		SvnDiffstatThread(SvnConnection *connection, JobQueue<std::string, Diffstat> *queue);
		~SvnDiffstatThread();

		static Diffstat diffstat(SvnConnection *c, svn_revnum_t r1, svn_revnum_t r2, apr_pool_t *pool);

	protected:
		void run();

	private:
		SvnConnection *d;
		JobQueue<std::string, Diffstat> *m_queue;
};


#endif // SUBVERSION_BACKEND_P_H_
