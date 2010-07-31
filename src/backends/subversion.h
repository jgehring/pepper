/*
 * pepper - SCM statistics report generator
 * Copyright (C) 2010 Jonas Gehring
 *
 * file: subversion.h
 * Subversion repository backend (interface)
 */


#ifndef SUBVERSION_BACKEND_H_
#define SUBVERSION_BACKEND_H_


#include "backend.h"


class SubversionBackend : public Backend
{
	public:
		SubversionBackend(const Options &options);
		~SubversionBackend();

		std::string name() const { return "subversion"; }

		Revision *revision(const std::string &id);
};


#endif // SUBVERSION_BACKEND_H_
