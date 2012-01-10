/*
 * pepper - SCM statistics report generator
 * Copyright (C) 2010-2011 Jonas Gehring
 *
 * Released under the GNU General Public License, version 3.
 * Please see the COPYING file in the source distribution for license
 * terms and conditions, or see http://www.gnu.org/licenses/.
 *
 * file: strlib.cpp
 * String handling functions
 */


#include <cstdarg>

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
std::string printf(const char *format, ...)
{
	va_list vl;
	va_start(vl, format);

	std::ostringstream os;

	const char *ptr = format-1;
	int lmod = 0;
	while (*(++ptr) != '\0') {
		if (*ptr != '%') {
			os << *ptr;
			continue;
		}

format:
		if (*(++ptr) == '\0') {
			break;
		}

		// Only a subset of format specifiers is supported
		switch (*ptr) {
			case 'd':
			case 'i':
				if (lmod > 1)  os << va_arg(vl, int64_t);
				else if (lmod > 0) os << va_arg(vl, long int);
				else os << va_arg(vl, int);
				break;

			case 'u':
				if (lmod > 1)  os << va_arg(vl, uint64_t);
				else if (lmod > 0) os << va_arg(vl, long unsigned int);
				else os << va_arg(vl, unsigned int);
				break;

			case 'c':
				os << (unsigned char)va_arg(vl, int);
				break;

			case 'e':
			case 'E':
			case 'f':
			case 'F':
			case 'g':
			case 'G':
				os << va_arg(vl, double);
				break;

			case 's':
				os << va_arg(vl, const char *);
				break;

			case '%':
				os << '%';
				break;

			case 'l':
				++lmod;
				goto format;
				break;

			default:
#ifndef NDEBUG
				throw PEX(std::string("Unknown format specifier ") + *ptr);
#endif
				break;
		}

		lmod = 0;
	}

	va_end(vl);
	return os.str();
}

} // namespace str
