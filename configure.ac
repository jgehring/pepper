#
# pepper - SCM statistics report generator
# Copyright (C) 2010 Jonas Gehring
#

AC_PREREQ([2.6])
AC_INIT(pepper, alpha, jonas.gehring@boolsoft.org)
AM_INIT_AUTOMAKE()
AC_CONFIG_HEADER([config.h])

sinclude(m4/configure_backends.m4)


#
# Checks for programs
#

AC_PROG_CXX()
AC_PROG_CC()
AC_PROG_INSTALL()


#
# Checks for functions
#

AC_FUNC_MALLOC()


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
