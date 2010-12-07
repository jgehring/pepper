/*
 * pepper - SCM statistics report generator
 * Copyright (C) 2010 Jonas Gehring
 *
 * file: syslib/fs.cpp
 * File system utility functions
 */


#include <cstdlib>
#include <cstdio>
#include <climits>

#include <sys/stat.h>

#include "fs.h"


namespace sys
{

namespace fs
{

// Like basename()
std::string basename(const std::string &path)
{
	if (path.empty()) {
		return std::string(".");
	}
	int i = path.length()-1;
	while (i > 0 && path[i] == '/') --i;
	int j = i;
	while (i > 0 && path[i] != '/') --i;
	if (i > 0) {
		return path.substr(i+1, j-i);
	}
	if (path[0] == '/') {
		if (path.length() == 1) {
			return path;
		}
		return path.substr(i+1, j-i);
	}
	return path;
}

// Like dirname()
std::string dirname(const std::string &path)
{
	if (path.empty()) {
		return std::string(".");
	}
	int i = path.length()-1;
	while (i > 0 && path[i] == '/') --i;
	while (i > 0 && path[i] != '/') --i;
	if (i > 0) {
		return path.substr(0, i);
	}
	if (path[0] == '/') {
		return path.substr(0, 1);
	}
	return std::string(".");
}

// Canonicalizes the given file path
std::string canonicalize(const std::string &path)
{
	// NOTE: glibc-only
	char cpath[PATH_MAX];
	if (realpath(path.c_str(), cpath) == NULL) {
		return std::string();
	}
	return std::string(cpath);
}

// Wrapper for mkdir()
int mkdir(const std::string &path)
{
	// Use current umask
	return ::mkdir(path.c_str(), 0777);
}

// mkdir() for a complete path
int mkpath(const std::string &path)
{
	std::string dir = dirname(path);
	struct stat statbuf;
	if (stat(dir.c_str(), &statbuf) == -1) {
		if (mkpath(dir) == -1) {
			return -1;
		}
	}
	return mkdir(path);
}

// Checks if the given file (or directory) exists
bool exists(const std::string &path)
{
	struct stat statbuf;
	return (stat(path.c_str(), &statbuf) != -1);
}

// Checks if the given regular file exists
bool fileExists(const std::string &path)
{
	struct stat statbuf;
	return (stat(path.c_str(), &statbuf) == 0 && S_ISREG(statbuf.st_mode));
}

// Checks if the given directory exists
bool dirExists(const std::string &path)
{
	struct stat statbuf;
	return (stat(path.c_str(), &statbuf) == 0 && S_ISDIR(statbuf.st_mode));
}

// Returns the size of the given file
size_t filesize(const std::string &path)
{
	struct stat statbuf;
	if (stat(path.c_str(), &statbuf) == -1) {
		return 0;
	}
	return statbuf.st_size;
}

} // namespace fs

} // namespace sys
