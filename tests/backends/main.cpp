/*
 * pepper - SCM statistics report generator
 * Copyright (C) 2010-2014 Jonas Gehring
 *
 * Released under the GNU General Public License, version 3.
 * Please see the COPYING file in the source distribution for license
 * terms and conditions, or see http://www.gnu.org/licenses/.
 *
 * file: tests/units/backend.cpp
 * Program entry point for the backend test program
 */


#include "catch_runner.hpp"
#include "catch.hpp"

#ifdef CONIFG_H
 #include "config.h"
#endif

#include <zlib.h>


typedef std::map<std::string, std::string> stringmap;


// Backend unit tests
#ifdef USE_SUBVERSION
 #include "test_subversion.h"
#endif


// Custom toString() implementations
namespace Catch
{
	template<>
	std::string toString<std::vector<std::string> >(const std::vector<std::string> &v)
	{
		std::ostringstream oss;
		oss << "std::vector( ";
		for (std::vector<std::string>::size_type i = 0; i < v.size(); i++) {
			oss << v[i];
			if (i < v.size()-1) oss << ", ";
		}
		oss << " )";
		return oss.str();
	}

	template<>
	std::string toString<std::vector<char> >(const std::vector<char> &v)
	{
		std::ostringstream oss;
		oss << "std::vector( ";
		oss << std::hex << std::showbase;
		for (std::vector<char>::size_type i = 0; i < v.size(); i++) {
			oss << (int)v[i];
			if (i < v.size()-1) oss << ", ";
		}
		oss << " )";
		return oss.str();
	}

	template<>
	std::string toString<stringmap>(const stringmap &v)
	{
		std::ostringstream oss;
		oss << "std::map( ";
		for (stringmap::const_iterator it = v.begin(); it != v.end(); ++it) {
			oss << it->first << " = " << it->second;

			stringmap::const_iterator jt(it);
			if (++jt != v.end()) oss << ", ";
		}
		oss << " )";
		return oss.str();
	}
}

// Program entry point
int main(int argc, char **argv)
{
	srand(time(NULL));
	int ret = Catch::Main(argc, argv);
	if (ret != 0) {
		return ret;
	}
#ifdef USE_SUBVERSION
	test_subversion::cleanup();
#endif
	return ret;
}
