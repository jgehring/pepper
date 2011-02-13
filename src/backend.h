/*
 * pepper - SCM statistics report generator
 * Copyright (C) 2010-2011 Jonas Gehring
 *
 * Released under the GNU General Public License, version 3.
 * Please see the COPYING file in the source distribution for license
 * terms and conditions, or see http://www.gnu.org/licenses/.
 *
 * file: backend.h
 * Abstract repository backend (interface)
 */


#ifndef BACKEND_H_
#define BACKEND_H_


#include <iostream>
#include <queue>
#include <string>
#include <vector>

#include "diffstat.h"
#include "tag.h"

#include "syslib/parallel.h"

class Options;
class Revision;


class Backend
{
	public:
		// Offers access to the repository history
		// The default implementation supports iterating through a pre-fetched vector
		class LogIterator : public sys::parallel::Thread
		{
			public:
				LogIterator(const std::vector<std::string> &ids = std::vector<std::string>());
				virtual ~LogIterator();

				virtual bool nextIds(std::queue<std::string> *queue);

			protected:
				virtual void run();

			protected:
				std::vector<std::string> m_ids;
				bool m_atEnd;
		};

	public:
		virtual ~Backend();

		static Backend *backendFor(const Options &options);
		static void listBackends(std::ostream &out = std::cout);

		virtual void init();

		virtual std::string name() const = 0;

		virtual std::string uuid() = 0;
		virtual std::string head(const std::string &branch = std::string()) = 0;
		virtual std::string mainBranch() = 0;
		virtual std::vector<std::string> branches() = 0;
		virtual std::vector<Tag> tags() = 0;
		virtual Diffstat diffstat(const std::string &id) = 0;
		virtual std::vector<std::string> tree(const std::string &id = std::string()) = 0;

		virtual LogIterator *iterator(const std::string &branch = std::string(), int64_t start = -1, int64_t end = -1) = 0;
		virtual void prefetch(const std::vector<std::string> &ids);
		virtual Revision *revision(const std::string &id) = 0;
		virtual void finalize();

		const Options &options() const;
		virtual void printHelp() const;

	protected:
		Backend(const Options &options);

	protected:
		const Options &m_opts;

	private:
		static Backend *backendForName(const std::string &name, const Options &options);
		static Backend *backendForUrl(const std::string &url, const Options &options);
};


#endif // BACKEND_H_
