/*
 * pepper - SCM statistics report generator
 * Copyright (C) 2010-2011 Jonas Gehring
 *
 * Released under the GNU General Public License, version 3.
 * Please see the COPYING file in the source distribution for license
 * terms and conditions, or see http://www.gnu.org/licenses/.
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
std::string makeAbsolute(const std::string &path);
std::string cwd();
void chdir(const std::string &path);
std::string escape(const std::string &base);

void mkdir(const std::string &path);
void mkpath(const std::string &path);
void unlink(const std::string &path);
void unlinkr(const std::string &path);
void rename(const std::string &oldpath, const std::string &newpath);
FILE *mkstemp(std::string *filename, const std::string &templ = std::string());

bool exists(const std::string &path);
bool fileExists(const std::string &path);
bool fileExecutable(const std::string &path);
bool dirExists(const std::string &path);
size_t filesize(const std::string &path);
std::string which(const std::string &program);

std::vector<std::string> ls(const std::string &path);

} // namespace fs

} // namespace sys


#endif // SYS_FS_H_
