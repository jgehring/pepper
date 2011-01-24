/*
 * pepper - SCM statistics report generator
 * Copyright (C) 2010-2011 Jonas Gehring
 *
 * Released under the GNU General Public License, version 3.
 * Please see the COPYING file in the source distribution for license
 * terms and conditions, or see http://www.gnu.org/licenses/.
 *
 * file: syslib/io.h
 * I/O classes and functions (interface)
 */


#ifndef SYS_IO_H_
#define SYS_IO_H_


#include <iostream>
#include <string>
#include <vector>


namespace sys
{

namespace io
{

bool isterm(FILE *f);

std::string execv(int *ret, const char * const *argv);
std::string exec(int *ret, const char *cmd, const char *arg1 = NULL, const char *arg2 = NULL, const char *arg3 = NULL, const char *arg4 = NULL, const char *arg5 = NULL, const char *arg6 = NULL, const char *arg7 = NULL);

class PopenStreambufData;
class PopenStreambuf : public std::streambuf
{
	public:
		explicit PopenStreambuf(const char *cmd, const char *arg1 = NULL, const char *arg2 = NULL, const char *arg3 = NULL, const char *arg4 = NULL, const char *arg5 = NULL, const char *arg6 = NULL, const char *arg7 = NULL);
		~PopenStreambuf();

		int close();

	private:
		int_type underflow();

	private:
		PopenStreambufData *d;
		const std::size_t m_putback;
		std::vector<char> m_buffer;

	private:
		// Not allowed
		PopenStreambuf(const PopenStreambuf &);
		PopenStreambuf &operator=(const PopenStreambuf &);
};

} // namespace io

} // namespace sys


#endif // SYS_IO_H_
