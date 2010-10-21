/*
 * pepper - SCM statistics report generator
 * Copyright (C) 2010 Jonas Gehring
 *
 * file: sysutils.h
 * Miscellaneous system utility functions
 */


#ifndef SYSUTILS_H_
#define SYSUTILS_H_


#include <string>


namespace SysUtils
{

std::string basename(const std::string &path);
std::string dirname(const std::string &path);

int mkdir(const std::string &path);
int mkpath(const std::string &path);

size_t filesize(const std::string &path);

} // namespace SysUtils


#endif // SYSUTILS_H_
