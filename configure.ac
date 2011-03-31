#
# pepper - SCM statistics report generator
# Copyright (C) 2010-2011 Jonas Gehring
#
# Released under the GNU General Public License, version 3.
# Please see the COPYING file in the source distribution for license
# terms and conditions, or see http://www.gnu.org/licenses/.
#

AC_PREREQ([2.6])
AC_INIT(pepper, 0.2, jonas.gehring@boolsoft.org)
AM_INIT_AUTOMAKE()
AC_CONFIG_HEADER([config.h])
AC_CONFIG_TESTDIR([tests])

# Custom arguments that should be listed first
AC_ARG_ENABLE([debug], [AS_HELP_STRING([--enable-debug], [Turn on debugging])], [debug="$enableval"], [debug="no"])
AM_CONDITIONAL([DEBUG], [test "x$debug" = "xyes"])

sinclude(m4/ax_check_zlib.m4)
sinclude(m4/ax_lua.m4)
sinclude(m4/ax_pthread.m4)
sinclude(m4/configure_backends.m4)
sinclude(m4/configure_features.m4)


#
# Checks for programs
#

AC_PROG_CXX()
AC_PROG_CC()
AC_PROG_INSTALL()


#
# Feature configuration
#

FEATURES_CHECK()

AM_CONDITIONAL([GNUPLOT], [test "x$gnuplot" = "xyes"])
AM_CONDITIONAL([MANPAGE], [test "x$manpage" = "xyes"])

# Build unit tests?
AC_ARG_ENABLE([tests], [AS_HELP_STRING([--enable-tests], [Build the test suite])], [testsuite="$enableval"], [testsuite="ifdebug"])
if test "x$testsuite" = "xifdebug" -a "x$debug" = "xyes"; then
	testsuite="yes"
fi
AM_CONDITIONAL([TESTSUITE], [test "x$testsuite" = "xyes"])


#
# Checks for system and compiler characteristics
#

AC_C_BIGENDIAN()

AC_CHECK_HEADER([CoreServices/CoreServices.h],
	[AC_DEFINE([HAVE_CORESERVICES], [1], [Define if you have the CoreServices API])
	 FRAMEWORKS="$FRAMEWORKS -framework CoreServices"])


#
# Checks for types
#

AC_TYPE_UINT64_T()


#
# Checks for headers
#

AC_CHECK_HEADERS([sys/time.h])
AC_HEADER_STDBOOL()


#
# Checks for functions
#

AC_FUNC_MALLOC()
AC_FUNC_MKTIME()
AC_CHECK_FUNCS([atexit getcwd gettimeofday memmove mkdir realpath setenv strtol strchr])
AC_FUNC_STRERROR_R()

if test "x$popen_noshell" = "xyes"; then
	AC_CHECK_FUNCS([strdup dup2 memset])
	AC_FUNC_ALLOCA()
	AC_FUNC_REALLOC()
	AC_FUNC_FORK()
fi


#
# Checks for libraries
#

AX_PTHREAD()

LUA_SUFFIXES="$with_lua_suffix"
if test "x$LUA_SUFFIXES" = "x"; then
	LUA_SUFFIXES="- 51 5.1"
fi
LUA_FOUND="no"
for with_lua_suffix in $LUA_SUFFIXES; do
	if test "$with_lua_suffix" = "-"; then
		with_lua_suffix=""
	fi
	AX_WITH_LUA()
	if test "x$LUA" = "x"; then
		continue
	fi
	AX_LUA_VERSION([501])
	AX_LUA_HEADERS()
	AX_LUA_HEADERS_VERSION([501])
	if test "x$LUA_HEADERS_IN_RANGE" != "xyes"; then
		continue
	fi
	AX_LUA_LIBS()
	if test "x$LUA_LIB" = "x"; then
		continue
	fi
	AC_SUBST([LUA_INCLUDE])
	AC_SUBST([LUA_LIB])
	LUA_FOUND="yes"
	break
done
if test "x$LUA_FOUND" = "xno"; then
	AC_MSG_ERROR([Lua could not be found. Please use the --with-lua-prefix option.])
fi

CHECK_ZLIB()


#
# Backend configuration
#

BACKENDS_CHECK()

AM_CONDITIONAL([GIT_BACKEND], [test "x$git" = "xyes"])
AM_CONDITIONAL([MERCURIAL_BACKEND], [test "x$mercurial" = "xyes"])
AM_CONDITIONAL([SVN_BACKEND], [test "x$subversion" = "xyes"])


#
# Debugging
#

if test "x$debug" = "xyes"; then
	CXXFLAGS="-O0 -g -DDEBUG"
	CFLAGS="-O0 -g -DDEBUG"
else
	CXXFLAGS="-O2 -DNDEBUG"
	CFLAGS="-O2 -DNDEBUG"
fi


#
# Write Makefiles
#

AC_SUBST([FRAMEWORKS])

AC_CONFIG_FILES([
	Makefile
	src/Makefile
	tests/Makefile
	tests/atlocal
	tests/diffstat/Makefile
	tests/units/Makefile
	tests/backends/Makefile
	reports/Makefile
	reports/pepper/Makefile
	docs/Makefile
])
AC_OUTPUT()


#
# Report
#

BACKENDS_REPORT()
FEATURES_REPORT()
echo
echo "Configuration finished, please run make now."
