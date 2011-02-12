dnl
dnl pepper - SCM statistics report generator
dnl Copyright (C) 2010-2011 Jonas Gehring
dnl
dnl Released under the GNU General Public License, version 3.
dnl Please see the COPYING file in the source distribution for license
dnl terms and conditions, or see http://www.gnu.org/licenses/.
dnl

AC_ARG_ENABLE([gnuplot], [AS_HELP_STRING([--disable-gnuplot], [Don't offer Gnuplot graphing to Lua scripts])], [gnuplot="$enableval"], [gnuplot="yes"])


dnl Run checks for the features 
AC_DEFUN([FEATURES_CHECK], [
])

dnl Print a feature configuration report
AC_DEFUN([FEATURES_REPORT], [
	echo
	echo "    Enabled(+) / disabled(-) features:"
	if test "x$gnuplot" = "xyes"; then echo "      + Gnuplot"; fi
	if test "x$gnuplot" = "xno"; then echo "      - Gnuplot"; fi
])
