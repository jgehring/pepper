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
		Report(const std::string &script, Backend *backend = NULL);
		Report(const std::string &script, const std::map<std::string, std::string> &options, Backend *backend = NULL);
		~Report();

		int run(std::ostream &err = std::cerr);
		void printHelp();

		static Report *current();
		Repository *repository() const;
		std::map<std::string, std::string> options() const;

		static void listReports(std::ostream &out = std::cout);

	private:
		Repository *m_repo;
		std::string m_script;
		std::map<std::string, std::string> m_options;

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
