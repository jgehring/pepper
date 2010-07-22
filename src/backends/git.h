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

		Revision *revision(const std::string &id);
};


#endif // GIT_BACKEND_H_
