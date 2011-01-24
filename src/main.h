/*
 * pepper - SCM statistics report generator
 * Copyright (C) 2010-2011 Jonas Gehring
 *
 * Released under the GNU General Public License, version 3.
 * Please see the COPYING file in the source distribution for license
 * terms and conditions, or see http://www.gnu.org/licenses/.
 *
 * file: main.h
 * Global header file
 */


#ifndef MAIN_H_
#define MAIN_H_


#include "config.h"


// Operating system detection (mostly from Qt, qglobal.h)
#if defined(__APPLE__) && (defined(__GNUC__) || defined(__xlC__) || defined(__xlc__))
 #define POS_DARWIN
#elif defined(__FreeBSD__) || defined(__DragonFly__) || defined(__NetBSD__) || defined(__OpenBSD__) || defined(__bsdi__)
 #define POS_BSD
#elif defined(__linux__) || defined(__linux)
 #define POS_LINUX
#elif defined(WIN64) || defined(_WIN64) || defined(__WIN64__) || defined(WIN32) || defined(_WIN32) || defined(__WIN32__) || defined(__NT__)
 #define POS_WIN
#endif


// Private variables can be declared like this to enable easy unit testing
#ifdef PEPPER_UNIT_TESTS
 #define PEPPER_PVARS public:
#else
 #define PEPPER_PVARS private:
#endif


// Standard integer types
#ifdef HAVE_STDINT_H
 #include <stdint.h>
#endif
#ifndef int8_t
 typedef signed char int8_t;
#endif
#ifndef uint8_t
 typedef unsigned char uint8_t;
#endif
#ifndef int16_t
 typedef signed short int16_t;
#endif
#ifndef uint16_t
 typedef unsigned short uint16_t;
#endif
#ifndef int32_t
 typedef signed int int32_t;
#endif
#ifndef uint32_t
 typedef unsigned int uint32_t;
#endif
#ifndef int64_t
 #if __WORDSIZE == 64
  typedef signed long int int64_t;
 #else
  __extension__
  typedef signed long long int int64_t;
 #endif
#endif
#ifndef uint64_t
 #if __WORDSIZE == 64
  typedef unsigned long int uint64_t;
 #else
  __extension__
  typedef unsigned long long int uint64_t;
 #endif
#endif


#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <exception>
#include <string>

#if defined(_GNU_SOURCE)
 #include <execinfo.h>
 #include <cxxabi.h>
#endif

namespace Pepper
{

class Exception : public std::exception
{
public:
	 Exception(const std::string &str, const char *file = NULL, int line = 0, const std::string &trace = std::string()) throw()
		 : m_str(str), m_trace(trace)
	 {
		 if (file) snprintf(m_where, sizeof(m_where), "%s:%d", file, line);
		 else m_where[0] = 0;
	 }

	 Exception(int code, const char *file = NULL, int line = 0, const std::string &trace = std::string()) throw()
		 : m_trace(trace)
	 {
		 if (file) snprintf(m_where, sizeof(m_where), "%s:%d", file, line);
		 else m_where[0] = 0;
		 char buf[512];
#if (_POSIX_C_SOURCE >= 200112L || _XOPEN_SOURCE >= 600) && !defined(_GNU_SOURCE)
		 m_str = std::string(strerror_r(code, buf, sizeof(buf)));
#else
		 strerror_r(code, buf, sizeof(buf));
		 m_str = std::string(buf);
#endif
	 }

	 virtual ~Exception() throw() { }

	 virtual const char *what() const throw() {
		 return m_str.c_str();
	 }
	 const char *where() const throw() {
		 return m_where;
	 }
	 const char *trace() const throw() {
		 return m_trace.c_str();
	 }

private:
	 std::string m_str;
	 std::string m_trace;
	 char m_where[512];
};

// Returns a formatted stack trace (with g++ only). Note that it should _not_ throw an exception!
// Credits go out to Timo Bingmann (http://idlebox.net/2008/0901-stacktrace-demangled/)
inline std::string stackTrace()
{
#if defined(_GNU_SOURCE)
	std::string str = "Stack trace:\n";

	// storage array for stack trace address data
	void* addrlist[60+1];
	// retrieve current stack addresses
	int addrlen = backtrace(addrlist, sizeof(addrlist) / sizeof(void*));

	if (addrlen == 0) {
		str += "    <empty, possibly corrupt>\n";
		return str;
	}

	// resolve addresses into strings containing "filename(function+address)",
	// this array must be free()-ed
	char** symbollist = backtrace_symbols(addrlist, addrlen);

	// allocate string which will be filled with the demangled function name
	size_t funcnamesize = 256;
	char *funcname = new char[funcnamesize];

	// iterate over the returned symbol lines. skip the first, it is the
	// address of this function.
	for (int i = 1; i < addrlen; i++)
	{
		char *begin_name = 0, *begin_offset = 0, *end_offset = 0;

		// find parentheses and +address offset surrounding the mangled name:
		// ./module(function+0x15c) [0x8048a6d]
		for (char *p = symbollist[i]; *p; ++p)
		{
			if (*p == '(')
				begin_name = p;
			else if (*p == '+')
				begin_offset = p;
			else if (*p == ')' && begin_offset) {
				end_offset = p;
				break;
			}
		}

		if (begin_name && begin_offset && end_offset && begin_name < begin_offset)
		{
			*begin_name++ = '\0';
			*begin_offset++ = '\0';
			*end_offset = '\0';

			// mangled name is now in [begin_name, begin_offset) and caller
			// offset in [begin_offset, end_offset). now apply
			// __cxa_demangle():

			int status;
			char* ret = abi::__cxa_demangle(begin_name, funcname, &funcnamesize, &status);
			if (status == 0) {
				funcname = ret; // use possibly realloc()-ed string
				str += "    ";
				str += std::string(symbollist[i])+": "+(const char *)funcname+"+"+(const char *)begin_offset;
			}
			else {
				// demangling failed. Output function name as a C function with
				// no arguments.
				str += "    ";
				str += std::string(symbollist[i])+": "+(const char *)begin_name+"()+"+(const char *)begin_offset;
			}
		}
		else
		{
			// couldn't parse the line? print the whole line.
			str += std::string("    ")+(const char *)symbollist[i];
		}
		str += '\n';
	}

	delete[] funcname;
	free(symbollist);
	return str;
#else
	return std::string();
#endif
}

} // namespace Pepper

// Generates an exception with where() information
#ifdef DEBUG
 #define PEX(str) Pepper::Exception(str, __FILE__, __LINE__, Pepper::stackTrace())
 #define PEX_ERRNO() Pepper::Exception(errno, __FILE__, __LINE__, Pepper::stackTrace())
#else
 #define PEX(str) Pepper::Exception(str, __FILE__, __LINE__)
 #define PEX_ERRNO() Pepper::Exception(errno, __FILE__, __LINE__)
#endif


#endif // MAIN_H_
