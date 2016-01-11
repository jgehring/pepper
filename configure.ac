#
# pepper - SCM statistics report generator
# Copyright (C) 2010-present Jonas Gehring
#
# Released under the GNU General Public License, version 3.
# Please see the COPYING file in the source distribution for license
# terms and conditions, or see http://www.gnu.org/licenses/.
#

AC_PREREQ([2.6])
AC_INIT(pepper, 0.3.3, jonas.gehring@boolsoft.org)
AM_INIT_AUTOMAKE([subdir-objects])
AC_CONFIG_HEADER([config.h])
AC_CONFIG_TESTDIR([tests])

# Custom arguments that should be listed first
AC_ARG_ENABLE([debug], [AS_HELP_STRING([--enable-debug], [Turn on debugging])], [debug="$enableval"], [debug="no"])
AC_ARG_ENABLE([tests], [AS_HELP_STRING([--enable-tests], [Build the test suite])], [testsuite="$enableval"], [testsuite="ifdebug"])

sinclude(m4/ax_check_zlib.m4)
sinclude(m4/ax_cxx_compile_stdcxx_11.m4)
sinclude(m4/ax_lua.m4)
sinclude(m4/ax_pthread.m4)
sinclude(m4/configure_backends.m4)
sinclude(m4/configure_features.m4)


#
# Checks for programs
#

AC_LANG([C++])
AX_CXX_COMPILE_STDCXX_11()
AC_PROG_INSTALL()
AC_PROG_RANLIB()


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
AC_CHECK_FUNCS([atexit getcwd gettimeofday memmove mkdir realpath setenv strtol strchr vsnprintf])
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
	LUA_SUFFIXES="- 51 5.1 51/lua"
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
	AX_LUA_VERSION([501],[503])
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
if test "x$want" = "xno"; then
	AC_MSG_ERROR([zlib is mandatory and cannot be disabled.])
fi


#
# Backend configuration
#

BACKENDS_CHECK()

AM_CONDITIONAL([GIT_BACKEND], [test "x$git" = "xyes"])
AM_CONDITIONAL([MERCURIAL_BACKEND], [test "x$mercurial" = "xyes"])
AM_CONDITIONAL([SVN_BACKEND], [test "x$subversion" = "xyes"])


#
# Feature configuration
#

FEATURES_CHECK()

AM_CONDITIONAL([GNUPLOT], [test "x$gnuplot" = "xyes"])
AM_CONDITIONAL([MANPAGE], [test "x$manpage" = "xyes"])
AM_CONDITIONAL([LEVELDB], [test "x$leveldb" = "xyes"])
AM_CONDITIONAL([BUILTIN_LEVELDB], [test "x$builtin_leveldb" = "xyes"])


#
# Debugging and unit tests
#

if test "x$debug" = "xyes"; then
	CXXFLAGS="${CXXFLAGS} -O0 -g -DDEBUG"
	CFLAGS="${CFLAGS} -O0 -g -DDEBUG"
else
	CXXFLAGS="-O2 ${CXXFLAGS} -DNDEBUG"
	CFLAGS="-O2 ${CFLAGS} -DNDEBUG"
fi
AM_CONDITIONAL([DEBUG], [test "x$debug" = "xyes"])

if test "x$testsuite" = "xifdebug" -a "x$debug" = "xyes"; then
	testsuite="yes"
fi
AM_CONDITIONAL([TESTSUITE], [test "x$testsuite" = "xyes"])

# Use -rdynamic for stack traces on non-OS X
AM_CONDITIONAL([RDYNAMIC], [test "x$debug" = "xyes" && test "x`uname`" != "xDarwin"])


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
	3rdparty/Makefile
])
AC_OUTPUT()


#
# Report
#

BACKENDS_REPORT()
FEATURES_REPORT()
echo
echo "Configuration finished, please run make now."
