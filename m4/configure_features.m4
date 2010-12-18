dnl
dnl pepper - SCM statistics report generator
dnl Copyright (C) 2010 Jonas Gehring
dnl

dnl Ivan Zahariev's popen-noshell
AC_ARG_ENABLE([popen-noshell], [AS_HELP_STRING([--enable-popen-noshell], [Use a faster but experimental version of popen() on Linux])], [popen_noshell="$enableval"], [popen_noshell="no"])
AC_ARG_ENABLE([gnuplot], [AS_HELP_STRING([--disable-gnuplot], [Don't offer Gnuplot graphing to Lua scripts])], [gnuplot="$enableval"], [gnuplot="yes"])


dnl Run checks for the features 
AC_DEFUN([FEATURES_CHECK], [
	if test "x$popen_noshell" != "xno"; then
		AC_CHECK_FUNC([clone], [popen_noshell="yes"], [popen_noshell="no"])
	fi
])


dnl Print a feature configuration report
AC_DEFUN([FEATURES_REPORT], [
	echo
	echo "    Enabled features:"
	if test "x$popen_noshell" = "xyes"; then echo "      * popen-noshell"; fi
	if test "x$gnuplot" = "xyes"; then echo "      * gnuplot"; fi
])
