dnl
dnl Search for Subversion libraries
dnl (roughly based on find_apr.m4)
dnl


AC_DEFUN([FIND_SVN], [
	svn_found="no"
	svn_prefix=""
 	AC_MSG_CHECKING([for Subversion])
	AC_ARG_WITH(svn, [AS_HELP_STRING([--with-svn=PATH],[prefix for installed Subversion development files])],
	[
		if test "$withval" = "no" || test "$withval" = "yes"; then
			AC_MSG_ERROR([--with-svn requires a directory or file to be provided])
		fi
		if test -r "$withval/include/subversion-1/svn_version.h"; then
			svn_major=`grep "#define SVN_VER_MAJOR" $withval/include/subversion-1/svn_version.h |sed s/[^0-9]*//`
			svn_minor=`grep "#define SVN_VER_MINOR" $withval/include/subversion-1/svn_version.h |sed s/[^0-9]*//`
			svn_patch=`grep "#define SVN_VER_PATCH" $withval/include/subversion-1/svn_version.h |sed s/[^0-9]*//`
			AC_MSG_RESULT([found version $svn_major.$svn_minor.$svn_patch])
			svn_found="yes"
			svn_prefix=$withval
		else
			AC_MSG_RESULT([no])
		fi
	],
	[
		for lookdir in /usr /usr/local /usr/local/svn /opt/svn; do
			if test -r "$lookdir/include/subversion-1/svn_version.h"; then
				svn_major=`grep "#define SVN_VER_MAJOR" $lookdir/include/subversion-1/svn_version.h |sed s/[^0-9]*//`
				svn_minor=`grep "#define SVN_VER_MINOR" $lookdir/include/subversion-1/svn_version.h |sed s/[^0-9]*//`
				svn_patch=`grep "#define SVN_VER_PATCH" $lookdir/include/subversion-1/svn_version.h |sed s/[^0-9]*//`
				AC_MSG_RESULT([found $svn_major.$svn_minor.$svn_patch in $lookdir])
				svn_found="yes"
				svn_prefix=$lookdir
				break
			fi
		done
		if test "$svn_found" = "no"; then
			AC_MSG_RESULT([no])
		fi
	]
	)

	if test "$svn_found" = "yes"; then
		SVN_CFLAGS="-I$svn_prefix/include/subversion-1"
		SVN_LDFLAGS="-L$svn_prefix/lib"
		SVN_PREFIX="$svn_prefix"
	fi
])

