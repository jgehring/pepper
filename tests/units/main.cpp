/*
 * pepper - SCM statistics report generator
 * Copyright (C) 2010-2012 Jonas Gehring
 *
 * Released under the GNU General Public License, version 3.
 * Please see the COPYING file in the source distribution for license
 * terms and conditions, or see http://www.gnu.org/licenses/.
 *
 * file: tests/units/main.cpp
 * Program entry point for the unit test program
 */


#include <cstring>
#include <map>
#include <string>
#include <vector>

#include "catch_runner.hpp"
#include "catch.hpp"


// Determine the number of test cases
#define NUM_INOUTS (sizeof(inout) / sizeof(inout_t))

typedef std::map<std::string, std::string> stringmap;

namespace tutils
{

void push_back(std::vector<std::string> *v, const char *s1 = 0, const char *s2 = 0, const char *s3 = 0, const char *s4 = 0, const char *s5 = 0, const char *s6 = 0, const char *s7 = 0, const char *s8 = 0)
{
	if (s1) v->push_back(s1);
	if (s2) v->push_back(s2);
	if (s3) v->push_back(s3);
	if (s4) v->push_back(s4);
	if (s5) v->push_back(s5);
	if (s6) v->push_back(s6);
	if (s7) v->push_back(s7);
	if (s8) v->push_back(s8);
}

} // namespace tutils


// Unit tests
#include "test_bstream.h"
#include "test_options.h"
#include "test_strlib.h"
#include "test_sys_fs.h"
#include "test_utils.h"


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
	return Catch::Main(argc, argv);
}
