/*
 * pepper - SCM statistics report generator
 * Copyright (C) 2010-present Jonas Gehring
 *
 * Released under the GNU General Public License, version 3.
 * Please see the COPYING file in the source distribution for license
 * terms and conditions, or see http://www.gnu.org/licenses/.
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
		static bool handles(const std::string &url);

		std::string uuid();
		std::string head(const std::string &branch = std::string());
		std::string mainBranch();
		std::vector<std::string> branches();
		std::vector<Tag> tags();
		DiffstatPtr diffstat(const std::string &id);
		std::vector<std::string> tree(const std::string &id = std::string());
		std::string cat(const std::string &path, const std::string &id = std::string());

		LogIterator *iterator(const std::string &branch = std::string(), int64_t start = -1, int64_t end = -1);
		Revision *revision(const std::string &id);

	private:
		std::string hgcmd() const;
		std::string hgcmd(const std::string &cmd, const std::string &args = std::string()) const;
		int simpleString(const std::string &str) const;
};


#endif // MERCURIAL_BACKEND_H_
