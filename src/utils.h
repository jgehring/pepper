/*
 * pepper - SCM statistics report generator
 * Copyright (C) 2010 Jonas Gehring
 *
 * file: utils.h
 * Miscellaneous utility functions
 */


#ifndef UTILS_H_
#define UTILS_H_


#include <cerrno>
#include <cstdarg>
#include <cstdio>
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
static bool str2int(const std::string &str, T *i)
{
	char *end;
	T val = strtoll(str.c_str(), &end, 0);

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

int64_t ptime(const std::string &str, const std::string &format);

void trim(std::string *str);
std::string trim(const std::string &str);
std::vector<std::string> split(const std::string &str, const std::string &token, bool trim = false);
std::string join(const std::vector<std::string> &v, const std::string &c = std::string());

std::string strprintf(const char *format, ...);

void printOption(const std::string &option, const std::string &text);

std::string execv(int *ret, const char * const *argv);
std::string exec(int *ret, const char *cmd, const char *arg1 = NULL, const char *arg2 = NULL, const char *arg3 = NULL, const char *arg4 = NULL, const char *arg5 = NULL, const char *arg6 = NULL, const char *arg7 = NULL);

std::vector<char> compress(const std::vector<char> &data, int level = 9);
std::vector<char> uncompress(const std::vector<char> &data);

} // namespace utils


#endif // UTILS_H_
