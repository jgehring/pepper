dnl
dnl FIND_SVN([MAJOR], [MINOR])
dnl
dnl Search for Subversion libraries
dnl (roughly based on find_apr.m4)
dnl

AC_DEFUN([FIND_SVN], [
	svn_found="no"
	svn_prefix=""
	ifelse([$1], [],
		[AC_MSG_CHECKING([for Subversion])],
		[ifelse([$2], [],
		[AC_MSG_CHECKING([for Subversion >= $1])],
		[AC_MSG_CHECKING([for Subversion >= $1.$2])]
	)])
	AC_ARG_WITH(svn, [AS_HELP_STRING([--with-svn=PATH],[prefix for installed Subversion development files])],
	[
		if test "$withval" = "no" || test "$withval" = "yes"; then
			AC_MSG_ERROR([--with-svn requires a directory or file to be provided])
		fi
		if test -r "$withval/include/subversion-1/svn_version.h"; then
			svn_major=`grep "#define SVN_VER_MAJOR" $withval/include/subversion-1/svn_version.h |sed s/[^0-9]*//`
			svn_minor=`grep "#define SVN_VER_MINOR" $withval/include/subversion-1/svn_version.h |sed s/[^0-9]*//`
			svn_patch=`grep "#define SVN_VER_PATCH" $withval/include/subversion-1/svn_version.h |sed s/[^0-9]*//`
			svn_prefix=$withval
			if test "x$1" != "x" && test "$svn_major" -lt "$1"; then
				AC_MSG_RESULT([no (found $svn_major.$svn_minor.$svn_patch)])
			elif test "x$2" != "x" && test "$svn_minor" -lt "$2"; then
				AC_MSG_RESULT([no (found $svn_major.$svn_minor.$svn_patch)])
			else
				AC_MSG_RESULT([found $svn_major.$svn_minor.$svn_patch])
				svn_found="yes"
			fi
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
				svn_prefix=$lookdir
				if test "x$1" != "x" && test "$svn_major" -lt "$1"; then
					AC_MSG_RESULT([no (found $svn_major.$svn_minor.$svn_patch in $lookdir)])
				elif test "x$2" != "x" && test "$svn_minor" -lt "$2"; then
					AC_MSG_RESULT([no (found $svn_major.$svn_minor.$svn_patch in $lookdir)])
				else
					AC_MSG_RESULT([found $svn_major.$svn_minor.$svn_patch in $lookdir])
					svn_found="yes"
				fi
				break
			fi
		done
		if test "x$svn_prefix" = "x"; then
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
