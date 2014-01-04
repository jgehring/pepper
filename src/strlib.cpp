/*
 * pepper - SCM statistics report generator
 * Copyright (C) 2010-2014 Jonas Gehring
 *
 * Released under the GNU General Public License, version 3.
 * Please see the COPYING file in the source distribution for license
 * terms and conditions, or see http://www.gnu.org/licenses/.
 *
 * file: strlib.cpp
 * String handling functions
 */


#include <cstdarg>
#include <cstdio>

#include "strlib.h"


namespace str
{

// Removes white-space characters at the beginning and end of a string
void trim(std::string *str)
{
	int start = 0;
	int end = str->length()-1;
	const char *data = str->c_str();

	while (start <= end && isspace(data[start])) {
		++start;
	}
	while (end >= start && isspace(data[end])) {
		--end;
	}

	if (start > 0 || end < (int)str->length()) {
		*str = str->substr(start, (end - start + 1));
	}
}

// Removes white-space characters at the beginning and end of a string
std::string trim(const std::string &str)
{
	std::string copy(str);
	trim(&copy);
	return copy;
}

// Split a string using the given token
std::vector<std::string> split(const std::string &str, const std::string &token, bool trim)
{
	std::vector<std::string> parts;
	size_t index = 0;

	if (str.length() == 0) {
		parts.push_back(str);
		return parts;
	}

	if (token.length() == 0) {
		for (size_t i = 0; i < str.length(); i++) {
			parts.push_back(trim ? str::trim(str.substr(i, 1)) : str.substr(i, 1));
		}
		return parts;
	}

	while (index < str.length()) {
		size_t pos = str.find(token, index);
		parts.push_back(trim ? str::trim(str.substr(index, pos - index)) : str.substr(index, pos - index));
		if (pos == std::string::npos) {
			break;
		}
		index = pos + token.length();
		if (index == str.length()) {
			parts.push_back("");
		}
	}

	return parts;
}

// Joins several strings
std::string join(const std::vector<std::string> &v, const std::string &c)
{
	std::string res;
	for (unsigned int i = 0; i < v.size(); i++) {
		res += v[i];
		if (i < v.size()-1) {
			res += c;
		}
	}
	return res;
}

// Joins several strings
std::string join(std::vector<std::string>::const_iterator start, std::vector<std::string>::const_iterator end, const std::string &c)
{
	std::string res;
	while (start != end) {
		res += *start;
		res += c;
		++start;
	}
	return res.substr(0, res.length()-c.length());
}

// Encloses a string in single quotes
std::string quote(const std::string &str)
{
	return std::string("'") + str + "'";
}

// Checks if a starts with b
bool startsWith(const std::string &a, const std::string &b)
{
	if (a.length() < b.length()) {
		return false;
	}
	return (a.compare(0, b.length(), b) == 0);
}

// Checks if a ends with b
bool endsWith(const std::string &a, const std::string &b)
{
	if (a.length() < b.length()) {
		return false;
	}
	return (a.compare(a.length() - b.length(), b.length(), b) == 0);
}

// sprintf for std::string
// Code sample originally form the vsnprintf man page:
// http://www.tin.org/bin/man.cgi?section=3&topic=vsnprintf
std::string printf(const char *format, ...)
{
	int size = 128;
	va_list ap;
	std::string str;

	// Try using vsnprintf() until the buffer is large enough
	while (true) {
		str.resize(size);

		va_start(ap, format);
		int n = vsnprintf((char *)str.c_str(), size, format, ap);
		va_end(ap);

		// Check result
		if (n > -1 && n < size) {
			str.resize(n);
			break;
		} else if (n > -1) {
			size = n + 1; // glibc 2.1: Returns amount of memory needed
		} else {
			n *= 2;
		}
	}

	return str;
}

} // namespace str
