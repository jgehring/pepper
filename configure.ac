#
# pepper - SCM statistics report generator
# Copyright (C) 2010 Jonas Gehring
#

AC_PREREQ([2.6])
AC_INIT(pepper, alpha, jonas.gehring@boolsoft.org)
AM_INIT_AUTOMAKE()
AC_CONFIG_HEADER([config.h])

sinclude(m4/configure_backends.m4)
sinclude(m4/find_lua.m4)
sinclude(m4/ax_check_zlib.m4)


#
# Checks for programs
#

AC_PROG_CXX()
AC_PROG_CC()
AC_PROG_INSTALL()


#
# Checks for types
#

AC_TYPE_UINT64_T()


#
# Checks for system and compiler characteristics
#

AC_C_BIGENDIAN


#
# Checks for functions
#

AC_FUNC_MALLOC()


#
# Checks for libraries
#

FIND_LUA()
if test "x$found_lua" != "xyes"; then
	AC_MSG_ERROR([Lua could not be located. Please check your pkg-config setup.])
fi
AC_SUBST([LUA_CFLAGS])
AC_SUBST([LUA_LIBS])

CHECK_ZLIB()


#
# Backend configuration
#

BACKENDS_CHECK()

AM_CONDITIONAL([GIT_BACKEND], [test "x$git" = "xyes"])
AM_CONDITIONAL([SVN_BACKEND], [test "x$subversion" = "xyes"])


#
# Write Makefiles
#

AC_CONFIG_FILES([
	Makefile
	src/Makefile
])
AC_OUTPUT()


#
# Report
#

BACKENDS_REPORT()
echo "Configuration finished, please run make now."
