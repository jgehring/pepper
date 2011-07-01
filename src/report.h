/*
 * pepper - SCM statistics report generator
 * Copyright (C) 2010-2011 Jonas Gehring
 *
 * Released under the GNU General Public License, version 3.
 * Please see the COPYING file in the source distribution for license
 * terms and conditions, or see http://www.gnu.org/licenses/.
 *
 * file: report.h
 * Report script context (interface)
 */


#ifndef REPORT_H_
#define REPORT_H_


#include <iostream>
#include <map>
#include <stack>
#include <string>

#include "lunar/lunar.h"

class Backend;
class Repository;


// Report context
class Report
{
	public:
		struct MetaData {
			std::string name;
			std::string description;
			std::map<std::string, std::string> options;
		};

	public:
		Report(const std::string &script, Backend *backend = NULL);
		Report(const std::string &script, const std::map<std::string, std::string> &options, Backend *backend = NULL);
		~Report();

		int run(std::ostream &out = std::cout, std::ostream &err = std::cerr);
		void printHelp();

		MetaData metaData();
		bool valid();
		std::ostream &out() const;
		bool outputRedirected() const;

		static Report *current();
		Repository *repository() const;
		std::map<std::string, std::string> options() const;

		static std::vector<std::pair<std::string, std::string> > listReports();
		static void printReportListing(std::ostream &out = std::cout);

	private:
		void readMetaData();

	private:
		Repository *m_repo;
		std::string m_script;
		std::map<std::string, std::string> m_options;
		std::ostream *m_out;
		MetaData m_metaData;
		bool m_metaDataRead;

		static std::stack<Report *> s_stack;

	// Lua binding
	public:
		Report(lua_State *L);

		int repository(lua_State *L);
		int getopt(lua_State *L);
		int run(lua_State *L);

		static const char className[];
		static Lunar<Report>::RegType methods[];
};


#endif // REPORT_H_
