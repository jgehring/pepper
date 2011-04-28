/*
 * pepper - SCM statistics report generator
 * Copyright (C) 2010-2011 Jonas Gehring
 *
 * Released under the GNU General Public License, version 3.
 * Please see the COPYING file in the source distribution for license
 * terms and conditions, or see http://www.gnu.org/licenses/.
 *
 * file: utils.h
 * Miscellaneous utility functions
 */


#ifndef UTILS_H_
#define UTILS_H_


#include <cerrno>
#include <cstdlib>
#include <limits>
#include <sstream>
#include <string>
#include <vector>

#include "main.h"


namespace utils
{

// Wrapper for strtol()
template <typename T>
static bool str2int(const std::string &str, T *i, int base = 0)
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

// Converts an interger to a string
template <typename T>
std::string int2str(T i)
{
	std::stringstream out;
	out << i;
	return out.str();
}

void trim(std::string *str);
std::string trim(const std::string &str);
std::vector<std::string> split(const std::string &str, const std::string &token, bool trim = false);
std::string join(const std::vector<std::string> &v, const std::string &c = std::string());
std::string join(std::vector<std::string>::const_iterator start, std::vector<std::string>::const_iterator end, const std::string &c = std::string());
std::string quote(const std::string &str);

std::string strprintf(const char *format, ...);

std::vector<char> compress(const std::vector<char> &data, int level = 9);
std::vector<char> uncompress(const std::vector<char> &data);

inline std::string childId(const std::string &id) {
	size_t p = id.find_last_of(':');
	return (p != std::string::npos ? id.substr(p+1) : id);
}

uint32_t crc32(const char *data, size_t len);
inline uint32_t crc32(const std::vector<char> &data) {
	return crc32(&data[0], data.size());
}

} // namespace utils


#endif // UTILS_H_
