/*
 * pepper - SCM statistics report generator
 * Copyright (C) 2010 Jonas Gehring
 *
 * file: utils.cpp
 * Miscellaneous utility functions
 */


#include <iostream>

#include <sys/time.h>

#include "utils.h"


namespace utils
{

// Wrapper for strptime()
int64_t ptime(const std::string &str, const std::string &format)
{
	struct tm tm;
	if (strptime(str.c_str(), format.c_str(), &tm) == NULL) {
		return -1;
	}
	return mktime(&tm);
}

// Removes white-space characters at the beginning and end of a string
void trim(std::string *str)
{
	int start = 0;
	int end = str->length()-1;

	while (start < end && isspace(str->at(start))) {
		++start;
	}
	while (end > start && isspace(str->at(end))) {
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

	if (token.length() == 0) {
		for (size_t i = 0; i < str.length(); i++) {
			parts.push_back(trim ? utils::trim(str.substr(i, 1)) : str.substr(i, 1));
		}
		return parts;
	}

	while (index < str.length()) {
		size_t pos = str.find(token, index);
		parts.push_back(trim ? utils::trim(str.substr(index, pos - index)) : str.substr(index, pos - index));
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

// sprintf for std::string
std::string strprintf(const char *format, ...)
{
	va_list vl;
	va_start(vl, format);

	std::ostringstream os;

	const char *ptr = format-1;
	while (*(++ptr) != '\0') {
		if (*ptr != '%') {
			os << *ptr;
			continue;
		}

		++ptr;

		// Only a subset of format specifiers is supported
		switch (*ptr) {
			case 'd':
			case 'i':
				os << va_arg(vl, int);
				break;

			case 'u':
				os << va_arg(vl, unsigned);
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

			default:
#ifndef NDEBUG
				std::cerr << "Error in strprintf(): unknown format specifier " << *ptr << std::endl;
				exit(1);
#endif
				break;
		}
	}

	va_end(vl);
	return os.str();
}

// Pretty-prints a help screen option
void printOption(const std::string &option, const std::string &text)
{
	std::cout << "  " << option;
	if (option.length() < 30) {
		for (int i = option.length(); i < 32; i++) {
			std::cout << " ";
		}
	} else {
		std::cout << std::endl;
		for (int i = 0; i < 34; i++) {
			std::cout << " ";
		}
	}
	std::cout << text << std::endl;
}

// Runs the specified command and returns the output
std::string exec(const std::string &cmd, int *ret)
{
	FILE *pipe = popen(cmd.c_str(), "r");
	if (!pipe) {
		throw PEX(strprintf("Unable to open pipe for command %s", cmd.c_str()));
	}

	char buffer[128];
	std::string result;
	while (!feof(pipe)) {
		if (fgets(buffer, 128, pipe) != NULL) {
			result += buffer;
		}
	}

	int r = pclose(pipe);
	if (ret != NULL) {
		*ret = r;
	}
	return result;
}

} // namespace utils
