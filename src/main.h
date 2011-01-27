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


#include "pex.h"


#endif // MAIN_H_
