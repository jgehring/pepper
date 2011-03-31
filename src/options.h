/*
 * pepper - SCM statistics report generator
 * Copyright (C) 2010-2011 Jonas Gehring
 *
 * Released under the GNU General Public License, version 3.
 * Please see the COPYING file in the source distribution for license
 * terms and conditions, or see http://www.gnu.org/licenses/.
 *
 * file: options.h
 * Command-line option parsing (interface)
 */


#ifndef OPTIONS_H_
#define OPTIONS_H_


#include "main.h"

#include <iostream>
#include <map>
#include <string>
#include <vector>


class Options
{
	public:
		Options();

		void parse(int argc, char **argv);

		bool helpRequested() const;
		bool versionRequested() const;
		bool backendListRequested() const;
		bool reportListRequested() const;

		bool useCache() const;
		std::string cacheDir() const;

		std::string forcedBackend() const;
		std::string repository() const;
		std::string value(const std::string &key, const std::string &defvalue = std::string()) const;
		std::map<std::string, std::string> options() const;

		std::string report() const;
		std::map<std::string, std::string> reportOptions() const;

		static void print(const std::string &option, const std::string &text, std::ostream &out = std::cout);
		static void printHelp(std::ostream &out = std::cout);

	private:
		void reset();
		void parse(const std::vector<std::string> &args);
		bool parseOpt(const std::string &arg, std::string *key, std::string *value);

	PEPPER_PVARS:
		std::map<std::string, std::string> m_options;
		std::map<std::string, std::string> m_reportOptions;
};


#endif // OPTIONS_H_
