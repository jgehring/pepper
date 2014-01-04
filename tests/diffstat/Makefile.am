#
# pepper - SCM statistics report generator
# Copyright (C) 2010-2014 Jonas Gehring
#
# Released under the GNU General Public License, version 3.
# Please see the COPYING file in the source distribution for license
# terms and conditions, or see http://www.gnu.org/licenses/.
#

noinst_PROGRAMS = pdiffstat
pdiffstat_SOURCES = \
	main.cpp

AM_CXXFLAGS = \
	-Wall -W -pipe \
	$(PTHREAD_CFLAGS)
AM_CPPFLAGS = \
	-DPEPPER_UNIT_TESTS \
	$(LUA_INCLUDE)
AM_CPPFLAGS += \
	-I$(top_srcdir)/src \
	-I$(top_srcdir)/src/3rdparty
pdiffstat_LDADD = $(top_builddir)/src/libpepper.a
LIBS += \
	$(PTHREAD_LIBS) \
	$(LUA_LIB) \
	$(FRAMEWORKS)


# Last but not least, the CFLAGS
AM_CFLAGS = $(AM_CXXFLAGS)
