/*
 * pepper - SCM statistics report generator
 * Copyright (C) 2010-2011 Jonas Gehring
 *
 * Released under the GNU General Public License, version 3.
 * Please see the COPYING file in the source distribution for license
 * terms and conditions, or see http://www.gnu.org/licenses/.
 *
 * file: pex.cpp
 * Custom exception class and macros
 */


// Enforce XSI-compliant strerror_r()
#if !defined(_XOPEN_SOURCE) || _XOPEN_SOURCE<600
 #undef _XOPEN_SOURCE
 #define _XOPEN_SOURCE 600
#endif

#include "main.h"

#include <cstdio>
#include <cstdlib>
#include <cstring>

#if defined(__GLIBC__)
 #include <cxxabi.h>
 #include <execinfo.h>
#endif

#include "pex.h"


// Constructor
PepperException::PepperException(const std::string &str, const char *file, int line, const std::string &trace) throw()
	: m_str(str), m_trace(trace)
{
	if (file) {
		snprintf(m_where, sizeof(m_where), "%s:%d", file, line);
	} else {
		m_where[0] = 0;
	}
}

// Constructor
PepperException::PepperException(int code, const char *file, int line, const std::string &trace) throw()
	: m_trace(trace)
{
	if (file) {
		snprintf(m_where, sizeof(m_where), "%s:%d", file, line);
	} else {
		m_where[0] = 0;
	}
	m_str = strerror(code);
}

// Destructor
PepperException::~PepperException() throw()
{

}

// Queries
const char *PepperException::what() const throw()
{
	 return m_str.c_str();
}
const char *PepperException::where() const throw()
{
	return m_where;
}
const char *PepperException::trace() const throw()
{
	return m_trace.c_str();
}

// Returns a formatted stack trace (with g++ only). Note that it should _not_ throw an exception!
// Credits go out to Timo Bingmann (http://idlebox.net/2008/0901-stacktrace-demangled/)
std::string PepperException::stackTrace()
{
#if defined(__GLIBC__)
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

// Wrapper for strerror_r
std::string PepperException::strerror(int code)
{
	std::string str;
	char buf[512];
#ifdef HAVE_STRERROR_R
#ifdef _GNU_SOURCE
	str = std::string(strerror_r(code, buf, sizeof(buf)));
#else
	if (strerror_r(code, buf, sizeof(buf)) == 0) {
		str = std::string(buf);
	} else {
		sprintf(buf, "System error code %d", code);
		str = std::string(buf);
	}
#endif
#else
	sprintf(buf, "System error code %d", code);
	str = std::string(buf);
#endif
	return str;
}
