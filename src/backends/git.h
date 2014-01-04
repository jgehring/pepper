/*
 * pepper - SCM statistics report generator
 * Copyright (C) 2010-2014 Jonas Gehring
 *
 * Released under the GNU General Public License, version 3.
 * Please see the COPYING file in the source distribution for license
 * terms and conditions, or see http://www.gnu.org/licenses/.
 *
 * file: git.h
 * Git repository backend (interface)
 */


#ifndef GIT_BACKEND_H_
#define GIT_BACKEND_H_


#include "backend.h"

class GitRevisionPrefetcher;


class GitBackend : public Backend
{
	public:
		GitBackend(const Options &options);
		~GitBackend();

		void init();
		void close();

		std::string name() const { return "git"; }
		static bool handles(const std::string &url);

		std::string uuid();
		std::string head(const std::string &branch = std::string());
		std::string mainBranch();
		std::vector<std::string> branches();
		std::vector<Tag> tags();
		Diffstat diffstat(const std::string &id);
		std::vector<std::string> tree(const std::string &id = std::string());
		std::string cat(const std::string &path, const std::string &id = std::string());

		LogIterator *iterator(const std::string &branch = std::string(), int64_t start = -1, int64_t end = -1);
		void prefetch(const std::vector<std::string> &ids);
		Revision *revision(const std::string &id);
		void finalize();

	private:
		std::string m_gitpath;
		GitRevisionPrefetcher *m_prefetcher;
};


#endif // GIT_BACKEND_H_
