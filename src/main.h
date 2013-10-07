/*
 * pepper - SCM statistics report generator
 * Copyright (C) 2010-2012 Jonas Gehring
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
 #define POS_BSD
#elif defined(__FreeBSD__) || defined(__DragonFly__) || defined(__NetBSD__) || defined(__OpenBSD__) || defined(__bsdi__)
 #define POS_BSD
#elif defined(__linux__) || defined(__linux)
 #define POS_LINUX
#elif defined(WIN64) || defined(_WIN64) || defined(__WIN64__) || defined(WIN32) || defined(_WIN32) || defined(__WIN32__) || defined(__NT__)
 #define POS_WIN
#endif


// Private variables can be declared like this to enable easy unit testing
#ifdef PEPPER_UNIT_TESTS
 #define PEPPER_PVARS public
 #define PEPPER_PROTVARS public
#else
 #define PEPPER_PVARS private
 #define PEPPER_PROTVARS protected
#endif


// Standard integer types
#include <stdint.h>

// Custom exception class
#include "pex.h"


#endif // MAIN_H_
