/*
 * pepper - SCM statistics report generator
 * Copyright (C) 2010 Jonas Gehring
 *
 * file: main.h
 * Global header file
 */


#ifndef MAIN_H_
#define MAIN_H_


#include "config.h"


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


#include <exception>
#include <string>
#include <cstdio>

namespace Pepper
{

class Exception : public std::exception
{
public:
	 Exception(const std::string &str, const char *file = NULL, int line = 0) throw()
		 : m_str(str)
	 {
		 if (file) snprintf(m_where, sizeof(m_where), "%s:%d", file, line);
		 else m_where[0] = 0;
	 }
	 virtual ~Exception() throw() { }

	 virtual const char *what() const throw() {
		 return m_str.c_str();
	 }
	 const char *where() const throw() {
		 return m_where;
	 }

private:
	 std::string m_str;
	 char m_where[512];
};

} // namespace Pepper

// Generates an exception with where() information
#define PEX(str) Pepper::Exception(str, __FILE__, __LINE__)


#endif // MAIN_H_
