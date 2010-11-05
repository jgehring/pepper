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


class GitBackend : public Backend
{
	public:
		GitBackend(const Options &options);
		~GitBackend();

		std::string name() const { return "git"; }

		std::string uuid();
		std::string head(const std::string &branch = std::string());
		std::string mainBranch();
		std::vector<std::string> branches();
		Diffstat diffstat(const std::string &id);

		RevisionIterator *iterator(const std::string &branch = std::string());
		Revision *revision(const std::string &id);
};


#endif // GIT_BACKEND_H_
