#
# pepper - SCM statistics report generator
# Copyright (C) 2010 Jonas Gehring
#

AC_PREREQ([2.6])
AC_INIT(pepper, 0.1, jonas.gehring@boolsoft.org)
AM_INIT_AUTOMAKE()
AC_CONFIG_HEADER([config.h])
AC_CONFIG_TESTDIR([tests])

# Custom arguments that should be listed first
AC_ARG_ENABLE([debug], [AS_HELP_STRING([--enable-debug], [Turn on debugging])], [debug="$enableval"], [debug="no"])

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

AM_CONDITIONAL([POPEN_NOSHELL], [test "x$popen_noshell" = "xyes"])
AM_CONDITIONAL([GNUPLOT], [test "x$gnuplot" = "xyes"])


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

if test "x$popen_noshell" = "xyes"; then
	AC_TYPE_PID_T()
	AC_TYPE_SIZE_T()
fi


#
# Checks for headers
#

AC_CHECK_HEADERS([sys/time.h])
AC_HEADER_STDBOOL()

if test "x$popen_noshell" = "xyes"; then
	AC_CHECK_HEADERS([fcntl.h])
fi


#
# Checks for functions
#

AC_FUNC_MALLOC()
AC_FUNC_MKTIME()
AC_CHECK_FUNCS([atexit getcwd gettimeofday memmove mkdir realpath setenv strtol strchr])

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

AX_WITH_LUA()
if test "x$LUA" = "x"; then
	AC_MSG_ERROR([Lua could not be found. Please use the --with-lua-prefix option.])
fi
AX_LUA_VERSION([501])
AX_LUA_HEADERS()
AX_LUA_HEADERS_VERSION([501])
AX_LUA_LIBS()
if test "x$LUA_LIB" = "x"; then
	AC_MSG_ERROR([Lua could not be found. Please use the --with-lua-prefix option.])
fi
AC_SUBST([LUA_INCLUDE])
AC_SUBST([LUA_LIB])

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
	CFLAGS="-O2 -DDEBUG"
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
	tests/options/Makefile
	reports/Makefile
])
AC_OUTPUT()


#
# Report
#

BACKENDS_REPORT()
FEATURES_REPORT()
echo
echo "Configuration finished, please run make now."
