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

class Options;
class Revision;


class Backend
{
	public:
		// Default implementation for a simple list of revision IDs
		class RevisionIterator
		{
			public:
				RevisionIterator();
				virtual ~RevisionIterator();

				virtual bool atEnd() const;
				virtual std::string next();

			protected:
				std::vector<std::string> m_ids;
				unsigned int m_index;
		};

	public:
		virtual ~Backend();

		static Backend *backendFor(const Options &options);

		virtual void init();

		virtual std::string name() const = 0;
		virtual std::string head(const std::string &branch = std::string()) = 0;
		virtual std::vector<std::string> branches() = 0;
		virtual Diffstat diffstat(const std::string &id) = 0;

		virtual RevisionIterator *iterator(const std::string &branch = std::string()) = 0;
		virtual Revision *revision(const std::string &id) = 0;

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
