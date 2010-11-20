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
		class SvnLogIterator : public LogIterator
		{
			public:
				SvnLogIterator(const std::string &url, const std::string &prefix, const Options::AuthData &auth, long int head);
				~SvnLogIterator();

				std::vector<std::string> nextIds();

			protected:
				void run();

			private:
				SvnConnection *d;
				std::string m_prefix;
				int64_t m_head;
				sys::thread::Mutex m_mutex;
				sys::thread::WaitCondition m_cond;
				std::vector<std::string>::size_type m_index;
				bool m_finished;
		};

	public:
		SubversionBackend(const Options &options);
		~SubversionBackend();

		void init();

		std::string name() const { return "subversion"; }
		static bool handles(const std::string &url);

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
