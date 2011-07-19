/*
 * pepper - SCM statistics report generator
 * Copyright (C) 2010-2011 Jonas Gehring
 *
 * Released under the GNU General Public License, version 3.
 * Please see the COPYING file in the source distribution for license
 * terms and conditions, or see http://www.gnu.org/licenses/.
 *
 * file: strlib.h
 * String handling functions (interface)
 */


#ifndef STRLIB_H_
#define STRLIB_H_


#include <cerrno>
#include <cstdlib>
#include <limits>
#include <sstream>
#include <streambuf>
#include <string>
#include <vector>

#include "main.h"


namespace str
{

// Wrapper for strtol()
template <typename T>
bool stoi(const std::string &str, T *i, int base = 0)
{
	char *end;
	T val = strtoll(str.c_str(), &end, base);

	if (errno == ERANGE || str.c_str() == end
	    || val > std::numeric_limits<T>::max()
	    || val < std::numeric_limits<T>::min()) {
		return false;
	}

	*i = (T)val;
	return true;
}
template <typename T>
inline bool str2int(const std::string &str, T *i, int base = 0) {
	return stoi<T>(str, i, base);
}

// Converts an interger to a string
template <typename T>
std::string itos(T i)
{
	std::stringstream out;
	out << i;
	return out.str();
}
template <typename T>
inline std::string int2str(T i) {
	return itos<T>(i);
}


void trim(std::string *str);
std::string trim(const std::string &str);
std::vector<std::string> split(const std::string &str, const std::string &token, bool trim = false);
std::string join(const std::vector<std::string> &v, const std::string &c = std::string());
std::string join(std::vector<std::string>::const_iterator start, std::vector<std::string>::const_iterator end, const std::string &c = std::string());
std::string quote(const std::string &str);

std::string printf(const char *format, ...);

} // namespace str


#endif // STRLIB_H_
