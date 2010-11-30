/*
 * pepper - SCM statistics report generator
 * Copyright (C) 2010 Jonas Gehring
 *
 * file: mercurial.h
 * Mercurial repository backend (interface)
 */


#ifndef MERCURIAL_BACKEND_H_
#define MERCURIAL_BACKEND_H_


#include "backend.h"


class MercurialBackend : public Backend
{
	public:
		MercurialBackend(const Options &options);
		~MercurialBackend();

		void init();

		std::string name() const { return "mercurial"; }

		std::string uuid();
		std::string head(const std::string &branch = std::string());
		std::string mainBranch();
		std::vector<std::string> branches();
		Diffstat diffstat(const std::string &id);

		LogIterator *iterator(const std::string &branch = std::string());
		Revision *revision(const std::string &id);

	private:
		std::string hgcmd() const;
		std::string hgcmd(const std::string &cmd, const std::string &args) const;
};


#endif // MERCURIAL_BACKEND_H_
