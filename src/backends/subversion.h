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
class SvnDiffstatScheduler;


class SubversionBackend : public Backend
{
	public:
		class SubversionLogIterator : public LogIterator
		{
			public:
				SubversionLogIterator(SvnConnection *c, const std::string &prefix, long int head);
		};

	public:
		SubversionBackend(const Options &options);
		~SubversionBackend();

		void init();

		std::string name() const { return "subversion"; }

		std::string uuid();
		std::string head(const std::string &branch = std::string());
		std::string mainBranch();
		std::vector<std::string> branches();
		Diffstat diffstat(const std::string &id);

		LogIterator *iterator(const std::string &branch = std::string());
		void prefetch(const std::vector<std::string> &ids);
		Revision *revision(const std::string &id);
		void finalize();

	private:
		SvnConnection *d;
		SvnDiffstatScheduler *m_sched;
};


#endif // SUBVERSION_BACKEND_H_
