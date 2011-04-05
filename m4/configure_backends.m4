dnl
dnl pepper - SCM statistics report generator
dnl Copyright (C) 2010-2011 Jonas Gehring
dnl
dnl Released under the GNU General Public License, version 3.
dnl Please see the COPYING file in the source distribution for license
dnl terms and conditions, or see http://www.gnu.org/licenses/.
dnl

sinclude(m4/find_apr.m4)
sinclude(m4/find_svn.m4)
sinclude(m4/ax_python_devel.m4)

AC_ARG_ENABLE([git], [AS_HELP_STRING([--disable-git], [Don't include the git backend])], [git="$enableval"], [git="yes"])
AC_ARG_ENABLE([mercurial], [AS_HELP_STRING([--disable-mercurial], [Don't include the mercurial backend (default is autodetect)])], [mercurial="$enableval"], [mercurial="auto"])
AC_ARG_ENABLE([svn], [AS_HELP_STRING([--disable-svn], [Don't include the subversion backend (default is autodetect)])], [subversion="$enableval"], [subversion="auto"])

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
			FIND_SVN([1],[5])
			if test "x$svn_found" != "xyes"; then
				if test "x$subversion" != "xauto"; then
					AC_MSG_ERROR([Subversion could not be located. Please use the --with-svn option.])
				fi
				subversion="no"
			else
				OLD_LDFLAGS=$LDFLAGS
				OLD_LIBS=$LIBS
				LDFLAGS="-L$SVN_PREFIX/lib"
				LIBS=$LDFLAGS
				AC_CHECK_LIB([svn_fs-1], [svn_fs_initialize], ,[AC_MSG_ERROR([Neccessary Subversion libraries are missing])]) 
				AC_CHECK_LIB([svn_client-1], [svn_client_diff4], ,[AC_MSG_ERROR([Neccessary Subversion libraries are missing])]) 
				AC_CHECK_LIB([svn_ra-1], [svn_ra_get_uuid2], ,[AC_MSG_ERROR([Neccessary Subversion libraries are missing])]) 
				AC_CHECK_LIB([svn_subr-1], [svn_cmdline_setup_auth_baton], ,[AC_MSG_ERROR([Neccessary Subversion libraries are missing])]) 
				AC_CHECK_LIB([svn_diff-1], [svn_diff_file_diff_2], ,[AC_MSG_ERROR([Neccessary Subversion libraries are missing])]) 
				AC_CHECK_LIB([svn_delta-1], [svn_txdelta_apply], ,[AC_MSG_ERROR([Neccessary Subversion libraries are missing])]) 
				AC_CHECK_LIB([svn_repos-1], [svn_repos_create], ,[AC_MSG_ERROR([Neccessary Subversion libraries are missing])]) 
				LDFLAGS=$OLD_LDFLAGS
				SVN_LDFLAGS=$LIBS
				LIBS=$OLD_LIBS
				AC_SUBST(SVN_CFLAGS)
				AC_SUBST(SVN_LDFLAGS)
				AC_SUBST(SVN_LIBS)
				subversion="yes"
			fi
		fi
	fi

	if test "x$mercurial" != "xno"; then
		AX_PYTHON_DEVEL([>= '2.1'], "$mercurial")
		if test "x$python_found" != "xyes"; then
			if test "x$mercurial" != "xauto"; then
				AC_MSG_ERROR([Python could not be located. Please use the PYTHON_VERSION variable.])
			fi
			mercurial="no"
		else
			# Inspiration from Stephan Peijnik
			# http://blog.sp.or.at/2008/08/31/autoconf-and-python-checking-for-modules/
			AC_MSG_CHECKING(for mercurial Python module)
			MODVERSION=`$PYTHON -c "from mercurial import __version__; print __version__.version" 2> /dev/null`
			if test "x$?" != "x0"; then
				AC_MSG_RESULT(not found)
				if test "x$mercurial" != "xauto"; then
					AC_MSG_ERROR([The mercurial Python module could not be located.])
				fi
				mercurial="no"
			else
				mercurial="yes"
				AC_MSG_RESULT([found $MODVERSION])
			fi
		fi
	fi
])

dnl Print a backend configuration report
AC_DEFUN([BACKENDS_REPORT], [
	echo
	echo "    Enabled(+) / disabled(-) SCM backends:"
	if test "x$git" = "xyes"; then echo "      + Git"; fi
	if test "x$mercurial" = "xyes"; then echo "      + Mercurial"; fi
	if test "x$subversion" = "xyes"; then echo "      + Subversion"; fi
	if test "x$git" = "xno"; then echo "      - Git"; fi
	if test "x$mercurial" = "xno"; then echo "      - Mercurial"; fi
	if test "x$subversion" = "xno"; then echo "      - Subversion"; fi
])
