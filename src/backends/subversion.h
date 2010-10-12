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

class SvnConnection;


class SubversionBackend : public Backend
{
	public:
		SubversionBackend(const Options &options);
		~SubversionBackend();

		void init();

		std::string name() const { return "subversion"; }
		std::string head(const std::string &branch = std::string());
		std::vector<std::string> branches();

		Revision *revision(const std::string &id);

	private:
		SvnConnection *d;
};


#endif // SUBVERSION_BACKEND_H_
