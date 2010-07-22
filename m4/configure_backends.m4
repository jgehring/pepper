dnl
dnl pepper - SCM statistics report generator
dnl Copyright (C) 2010 Jonas Gehring
dnl

sinclude(m4/find_apr.m4)
sinclude(m4/find_svn.m4)

AC_ARG_ENABLE([git], [AS_HELP_STRING([--enable-git], [Include the git backend (default is yes)])], [git="$enableval"], [git="yes"])
AC_ARG_ENABLE([svn], [AS_HELP_STRING([--enable-svn], [Include the subversion backend (default is autodetect)])], [subversion="$enableval"], [subversion="auto"])

dnl Run checks for the backends
AC_DEFUN([BACKENDS_CHECK], [
	if test "x$subversion" != "xno"; then
		APR_FIND_APR(,,[1],[1])
		if test "$apr_found" = "no"; then
			if test "x$subversion" != "xauto"; then
				AC_MSG_ERROR([APR could not be located. Please use the --with-apr option.])
			fi
			subversion="no"
		else
			APR_BUILD_DIR="`$apr_config --installbuilddir`"
			dnl make APR_BUILD_DIR an absolute directory
			APR_BUILD_DIR="`cd $APR_BUILD_DIR && pwd`"
			APR_CFLAGS=`$apr_config --cflags`
			APR_CPPFLAGS=`$apr_config --cppflags`
			APR_INCLUDES="`$apr_config --includes`"
			APR_LIBS="`$apr_config --link-ld --libs`"
			AC_SUBST(APR_CFLAGS)
			AC_SUBST(APR_CPPFLAGS)
			AC_SUBST(APR_INCLUDES)
			AC_SUBST(APR_LIBS)
		fi

		if test "x$subversion" != "xno"; then
			FIND_SVN()
			if test "x$svn_found" != "xyes"; then
				if test "x$subversion" != "xauto"; then
					AC_MSG_ERROR([Subversion could not be located. Please use the --with-svn option.])
				fi
				subversion="no"
			else
				AC_SUBST(SVN_CFLAGS)
				AC_SUBST(SVN_LDFLAGS)
				subversion="yes"
			fi
		fi
	fi
])


dnl Print a backend configuration report
AC_DEFUN([BACKENDS_REPORT], [
	echo
	echo "    Enabled SCM backends:"
	if test "x$git" = "xyes"; then echo "      * git"; fi
	if test "x$subversion" = "xyes"; then echo "      * Subversion"; fi
	echo
])
