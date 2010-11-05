/*
 * pepper - SCM statistics report generator
 * Copyright (C) 2010 Jonas Gehring
 *
 * file: utils.h
 * Miscellaneous utility functions
 */


#ifndef UTILS_H_
#define UTILS_H_


#include <string>
#include <vector>

#include "main.h"


namespace utils
{

bool str2int(const std::string &str, int *i);
bool str2int(const std::string &str, unsigned int *i);
bool str2int(const std::string &str, long int *i);
std::string int2str(int i);
std::string int2str(long int i);

void trim(std::string *str);
std::string trim(const std::string &str);
std::vector<std::string> split(const std::string &str, const std::string &token, bool trim = false);

std::string strprintf(const char *format, ...);

void printOption(const std::string &option, const std::string &text);

} // namespace utils


#endif // UTILS_H_
