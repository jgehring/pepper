/*
 * pepper - SCM statistics report generator
 * Copyright (C) 2010 Jonas Gehring
 *
 * file: backend.h
 * Abstract repository backend (interface)
 */


#ifndef BACKEND_H_
#define BACKEND_H_


#include <string>
#include <vector>

#include "diffstat.h"

#include "sys/thread.h"

class Options;
class Revision;


class Backend
{
	public:
		// Offers access to the repository history
		// The default implementation supports iterating through a pre-fetched vector
		class LogIterator : public sys::thread::Thread
		{
			public:
				LogIterator(const std::vector<std::string> &ids = std::vector<std::string>());
				virtual ~LogIterator();

				virtual std::vector<std::string> nextIds();

			protected:
				virtual void run();

			protected:
				std::vector<std::string> m_ids;
				bool m_atEnd;
		};

	public:
		virtual ~Backend();

		static Backend *backendFor(const Options &options);

		virtual void init();

		virtual std::string name() const = 0;

		virtual std::string uuid() = 0;
		virtual std::string head(const std::string &branch = std::string()) = 0;
		virtual std::string mainBranch() = 0;
		virtual std::vector<std::string> branches() = 0;
		virtual Diffstat diffstat(const std::string &id) = 0;

		virtual LogIterator *iterator(const std::string &branch = std::string()) = 0;
		virtual void prefetch(const std::vector<std::string> &ids);
		virtual Revision *revision(const std::string &id) = 0;
		virtual void finalize();

		const Options &options() const;

	protected:
		Backend(const Options &options);

	protected:
		const Options &m_opts;

	private:
		static Backend *backendForName(const std::string &name, const Options &options);
		static Backend *backendForUrl(const std::string &url, const Options &options);
};


#endif // BACKEND_H_
