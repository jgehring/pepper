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

bool str2int(const std::string &str, int32_t *i);
bool str2int(const std::string &str, uint32_t *i);
bool str2int(const std::string &str, int64_t *i);
bool str2int(const std::string &str, long int *i);
std::string int2str(int32_t i);
std::string int2str(int64_t i);
std::string int2str(long int i);

int64_t ptime(const std::string &str, const std::string &format);

void trim(std::string *str);
std::string trim(const std::string &str);
std::vector<std::string> split(const std::string &str, const std::string &token, bool trim = false);
std::string join(const std::vector<std::string> &v, const std::string &c = std::string());

std::string strprintf(const char *format, ...);

void printOption(const std::string &option, const std::string &text);

std::string exec(const std::string &cmd, int *ret = NULL);

} // namespace utils


#endif // UTILS_H_
