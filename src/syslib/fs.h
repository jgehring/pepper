/*
 * pepper - SCM statistics report generator
 * Copyright (C) 2010 Jonas Gehring
 *
 * file: syslib/fs.h
 * File system utility functions (interface)
 */


#ifndef SYS_FS_H_
#define SYS_FS_H_


#include <string>
#include <vector>


namespace sys
{

namespace fs
{

std::string basename(const std::string &path);
std::string dirname(const std::string &path);
std::string canonicalize(const std::string &path);

int mkdir(const std::string &path);
int mkpath(const std::string &path);

bool exists(const std::string &path);
bool fileExists(const std::string &path);
bool dirExists(const std::string &path);
size_t filesize(const std::string &path);

std::vector<std::string> ls(std::string &path);

} // namespace fs

} // namespace sys


#endif // SYS_FS_H_
