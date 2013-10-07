/*
 * pepper - SCM statistics report generator
 * Copyright (C) 2010-2012 Jonas Gehring
 *
 * Released under the GNU General Public License, version 3.
 * Please see the COPYING file in the source distribution for license
 * terms and conditions, or see http://www.gnu.org/licenses/.
 *
 * file: syslib/fs.cpp
 * File system utility functions
 */


#include "main.h"

#include <cstdio>
#include <cstring>
#include <climits>

#include <dirent.h>
#include <unistd.h>

#include <sys/stat.h>

#include "strlib.h"

#include "fs.h"


namespace sys
{

namespace fs
{

// Like basename()
std::string basename(const std::string &path)
{
	if (path.empty()) {
		return std::string();
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
	return path.substr(0, j+1);
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
		throw PEX_ERRNO();
	}
	return std::string(cpath);
}

// Converts a possibly relative path to an absolute one
std::string makeAbsolute(const std::string &path)
{
	// Check for a URL scheme
	if (path.empty() || path.find("://") != std::string::npos) {
		return path;
	}

	// Make path absolute
	std::string abspath(path);
	if (abspath[0] != '/') {
		abspath = cwd() + "/" + path;
	}

	return canonicalize(abspath);
}

// Returns the current working directory
std::string cwd()
{
	char cwd[FILENAME_MAX];
	if (!getcwd(cwd, sizeof(cwd))) {
		throw PEX_ERRNO();
	}
	return std::string(cwd);
}

// Changes the current directory
void chdir(const std::string &path)
{
	if (::chdir(path.c_str()) != 0) {
		throw PEX_ERRNO();
	}
}

// Escapes invalid / problematique characters from the given base name
std::string escape(const std::string &base)
{
	// Replace most non-alphanumeric character with its hexadecimal
	// represetation, prefixed by '#'
	std::stringstream ss;
	ss << std::hex << std::uppercase;
	for (size_t i = 0; i < base.length(); i++) {
		if (isalnum(base[i]) || base[i] == '.' || base[i] == '_') {
			ss << base[i];
		} else {
			ss << '#' << int(base[i]);
		}
	}

	return ss.str();
}

// Wrapper for mkdir()
void mkdir(const std::string &path)
{
	// Use current umask
	if (::mkdir(path.c_str(), 0777) != 0) {
		throw PEX_ERRNO();
	}
}

// mkdir() for a complete path
void mkpath(const std::string &path)
{
	std::string dir = dirname(path);
	struct stat statbuf;
	if (stat(dir.c_str(), &statbuf) != 0) {
		mkpath(dir);
	}

	mkdir(path);
}

// Wrapper for unlink()
void unlink(const std::string &path)
{
	if (::unlink(path.c_str()) == -1) {
		throw PEX_ERRNO();
	}
}

// Recursive unlink()
void unlinkr(const std::string &path)
{
	struct stat statbuf;
	if (stat(path.c_str(), &statbuf) != 0) {
		return;
	}
	if (!S_ISDIR(statbuf.st_mode)) {
		unlink(path);
		return;
	}

	DIR *dp;
	if ((dp = opendir(path.c_str())) == NULL) {
		throw PEX_ERRNO();
	}

	struct dirent *ep;
	while ((ep = readdir(dp)) != NULL) {
		if (strcmp(ep->d_name, ".") && strcmp(ep->d_name, "..")) {
			unlinkr(path + "/" + ep->d_name);
		}
	}
	closedir(dp);

	if (::rmdir(path.c_str()) == -1) {
		throw PEX_ERRNO();
	}
}

// Wrapper for rename()
void rename(const std::string &oldpath, const std::string &newpath)
{
	if (::rename(oldpath.c_str(), newpath.c_str()) == -1) {
		throw PEX_ERRNO();
	}
}

// Wrapper for mkstemp()
FILE *mkstemp(std::string *filename, const std::string &templ)
{
	char buf[FILENAME_MAX+1];
	buf[FILENAME_MAX] = '\0';

	if (!templ.empty()) {
		strncpy(buf, templ.c_str(), FILENAME_MAX);
	} else {
		char *tmpdir = getenv("TMPDIR");
		snprintf(buf, FILENAME_MAX, "%s/pepperXXXXXX", (tmpdir ? tmpdir : "/tmp"));
	}

	int fd = ::mkstemp(buf);
	if (fd == -1) {
		throw PEX_ERRNO();
	}

	if (filename) {
		*filename = buf;
	}
	return fdopen(fd, "r+w");
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

// Checks if the given regular file exists and is executable
bool fileExecutable(const std::string &path)
{
	return (access(path.c_str(), X_OK) == 0);
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
		throw PEX_ERRNO();
	}
	return statbuf.st_size;
}

// Searches the current PATH for the given program
std::string which(const std::string &program)
{
	std::string progPath;
	char *path = getenv("PATH");
	if (path == NULL) {
		throw PEX("PATH is not set");
	}
	std::vector<std::string> ls;
#ifdef POS_WIN
	ls = str::split(path, ";");
#else
	ls = str::split(path, ":");
#endif
	for (size_t i = 0; i < ls.size(); i++) {
		std::string t = ls[i] + "/" + program;
#ifdef POS_WIN
		t += ".exe";
#endif
		if (sys::fs::fileExecutable(t)) {
			progPath = t;
			break;
		}
	}

	if (progPath.empty()) {
		throw PEX(std::string("Can't find ") + program + " in PATH");
	}
	return progPath;
}

// Lists the contents of the given directory
std::vector<std::string> ls(const std::string &path)
{
	DIR *dp;
	std::vector<std::string> entries;

	if ((dp = opendir(path.c_str())) == NULL) {
		throw PEX_ERRNO();
	}

	struct dirent *ep;
	while ((ep = readdir(dp)) != NULL) {
		if (strcmp(ep->d_name, ".") && strcmp(ep->d_name, "..")) {
			entries.push_back(ep->d_name);
		}
	}
	closedir(dp);

	return entries;
}

} // namespace fs

} // namespace sys
