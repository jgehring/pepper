/*
 * pepper - SCM statistics report generator
 * Copyright (C) 2010-2014 Jonas Gehring
 *
 * Released under the GNU General Public License, version 3.
 * Please see the COPYING file in the source distribution for license
 * terms and conditions, or see http://www.gnu.org/licenses/.
 *
 * file: pex.h
 * Custom exception class and macros (interface)
 */


#ifndef PEX_H_
#define PEX_H_


#include <cerrno>
#include <exception>
#include <string>


class PepperException : public std::exception
{
public:
	 PepperException(const std::string &str, const char *file = NULL, int line = 0, const std::string &trace = std::string()) throw();
	 PepperException(int code, const char *file = NULL, int line = 0, const std::string &trace = std::string()) throw();
	 virtual ~PepperException() throw();

	 virtual const char *what() const throw();
	 const char *where() const throw();
	 const char *trace() const throw();

	 static std::string stackTrace();
	 static std::string strerror(int code);

private:
	 std::string m_str;
	 std::string m_trace;
	 char m_where[512];
};


// Generates an exception with where() information
#ifdef DEBUG
 #define PEX(str) PepperException(str, __FILE__, __LINE__, PepperException::stackTrace())
 #define PEX_ERR(c) PepperException(c, __FILE__, __LINE__, PepperException::stackTrace())
 #define PEX_ERRNO() PepperException(errno, __FILE__, __LINE__, PepperException::stackTrace())
#else
 #define PEX(str) PepperException(str, __FILE__, __LINE__)
 #define PEX_ERR(c) PepperException(c, __FILE__, __LINE__)
 #define PEX_ERRNO() PepperException(errno, __FILE__, __LINE__)
#endif


#endif // PEX_H_
