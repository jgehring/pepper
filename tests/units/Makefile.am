#
# pepper - SCM statistics report generator
# Copyright (C) 2010-2011 Jonas Gehring
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
	test_utils.h

AM_CXXFLAGS = \
	-Wall -W -pipe \
	$(PTHREAD_CFLAGS)
AM_CPPFLAGS = \
	-DPEPPER_UNIT_TESTS
INCLUDES = \
	-I$(top_srcdir)/tests/catch \
	-I$(top_srcdir)/src \
	-I$(top_srcdir)/src/3rdparty
LIBS += \
	$(PTHREAD_LIBS) \
	$(LUA_LIB) \
	$(FRAMEWORKS) \
	$(top_builddir)/src/libpepper.a


# Last but not least, the CFLAGS
AM_CFLAGS = $(AM_CXXFLAGS)
