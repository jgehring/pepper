#
# pepper - SCM statistics report generator
# Copyright (C) 2010-2011 Jonas Gehring
#
# Released under the GNU General Public License, version 3.
# Please see the COPYING file in the source distribution for license
# terms and conditions, or see http://www.gnu.org/licenses/.
#

noinst_PROGRAMS = poptions
poptions_SOURCES = \
	main.cpp \
	\
	$(top_srcdir)/src/logger.cpp \
	$(top_srcdir)/src/options.cpp \
	$(top_srcdir)/src/pex.cpp \
	$(top_srcdir)/src/utils.cpp \
	$(top_srcdir)/src/syslib/fs.cpp \
	$(top_srcdir)/src/syslib/parallel.cpp

AM_CXXFLAGS = \
	-Wall -W -pipe \
	$(PTHREAD_CFLAGS)
AM_CPPFLAGS = \
	-DPEPPER_UNIT_TESTS
INCLUDES = \
	-I$(top_srcdir)/src \
	-I$(top_srcdir)/src/3rdparty
LIBS += \
	$(PTHREAD_LIBS) \
	$(FRAMEWORKS)


# Last but not least, the CFLAGS
AM_CFLAGS = $(AM_CXXFLAGS)
