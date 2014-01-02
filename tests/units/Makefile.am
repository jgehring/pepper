#
# pepper - SCM statistics report generator
# Copyright (C) 2010-2012 Jonas Gehring
#
# Released under the GNU General Public License, version 3.
# Please see the COPYING file in the source distribution for license
# terms and conditions, or see http://www.gnu.org/licenses/.
#

noinst_PROGRAMS = units
units_SOURCES = \
	main.cpp \
	test_bstream.h \
	test_options.h \
	test_strlib.h \
	test_sys_fs.h \
	test_utils.h \
	test_parallel.h

AM_CXXFLAGS = \
	-Wall -W -pipe \
	$(PTHREAD_CFLAGS)
AM_CPPFLAGS = \
	-DPEPPER_UNIT_TESTS
AM_CPPFLAGS += \
	-I$(top_srcdir)/tests/catch \
	-I$(top_srcdir)/src \
	-I$(top_srcdir)/src/3rdparty
units_LDADD = $(top_builddir)/src/libpepper.a
LIBS += \
	$(PTHREAD_LIBS) \
	$(LUA_LIB) \
	$(FRAMEWORKS)


# Last but not least, the CFLAGS
AM_CFLAGS = $(AM_CXXFLAGS)
