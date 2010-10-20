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
class SvnLogConnection;


class SubversionBackend : public Backend
{
	public:
		class SubversionRevisionIterator : public RevisionIterator
		{
			public:
				SubversionRevisionIterator(SvnConnection *c, const std::string &prefix, long int head);
		};

	public:
		SubversionBackend(const Options &options);
		~SubversionBackend();

		void init();

		std::string name() const { return "subversion"; }

		std::string uuid();
		std::string head(const std::string &branch = std::string());
		std::vector<std::string> branches();
		Diffstat diffstat(const std::string &id);

		RevisionIterator *iterator(const std::string &branch = std::string());
		Revision *revision(const std::string &id);

	private:
		SvnConnection *d;
};


#endif // SUBVERSION_BACKEND_H_
