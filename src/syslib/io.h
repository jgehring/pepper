/*
 * pepper - SCM statistics report generator
 * Copyright (C) 2010-2012 Jonas Gehring
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


#include <string>
#include <vector>
#include <streambuf>


namespace sys
{

namespace io
{

bool isterm(FILE *f);

std::string execv(int *ret, const char *cmd, const char * const *argv);
std::string exec(int *ret, const char *cmd, const char *arg1 = NULL, const char *arg2 = NULL, const char *arg3 = NULL, const char *arg4 = NULL, const char *arg5 = NULL, const char *arg6 = NULL, const char *arg7 = NULL);

class PopenStreambufData;
class PopenStreambuf : public std::streambuf
{
	public:
		explicit PopenStreambuf(const char *cmd, const char *arg1 = NULL, const char *arg2 = NULL, const char *arg3 = NULL, const char *arg4 = NULL, const char *arg5 = NULL, const char *arg6 = NULL, const char *arg7 = NULL, std::ios::open_mode m = std::ios::in);
		explicit PopenStreambuf(const char *cmd, const char * const *argv, std::ios::open_mode m = std::ios::in);
		~PopenStreambuf();

		int close();
		void closeWrite();

	private:
		int_type underflow();
		int_type sync();
		int_type overflow(int_type c);

	private:
		PopenStreambufData *d;
		std::ios::open_mode m_mode;
		const std::size_t m_putback;
		std::vector<char> m_inbuffer, m_outbuffer;

	private:
		// Not allowed
		PopenStreambuf(const PopenStreambuf &);
		PopenStreambuf &operator=(const PopenStreambuf &);
};

} // namespace io

} // namespace sys


#endif // SYS_IO_H_
