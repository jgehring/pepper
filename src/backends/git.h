/*
 * pepper - SCM statistics report generator
 * Copyright (C) 2010 Jonas Gehring
 *
 * file: git.h
 * Git repository backend (interface)
 */


#ifndef GIT_BACKEND_H_
#define GIT_BACKEND_H_


#include "backend.h"

class GitDiffstatPrefetcher;


class GitBackend : public Backend
{
	public:
		GitBackend(const Options &options);
		~GitBackend();

		void init();

		std::string name() const { return "git"; }
		static bool handles(const std::string &url);

		std::string uuid();
		std::string head(const std::string &branch = std::string());
		std::string mainBranch();
		std::vector<std::string> branches();
		std::vector<Tag> tags();
		Diffstat diffstat(const std::string &id);

		LogIterator *iterator(const std::string &branch = std::string());
		void prefetch(const std::vector<std::string> &ids);
		Revision *revision(const std::string &id);
		void finalize();

	private:
		GitDiffstatPrefetcher *m_prefetcher;
};


#endif // GIT_BACKEND_H_
