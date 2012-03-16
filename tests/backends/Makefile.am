#
# pepper - SCM statistics report generator
# Copyright (C) 2010-2012 Jonas Gehring
#
# Released under the GNU General Public License, version 3.
# Please see the COPYING file in the source distribution for license
# terms and conditions, or see http://www.gnu.org/licenses/.
#

noinst_PROGRAMS = backends
backends_SOURCES = \
	main.cpp

AM_CXXFLAGS = \
	-Wall -W -pipe \
	$(PTHREAD_CFLAGS)
AM_CPPFLAGS = \
	-DPEPPER_UNIT_TESTS
INCLUDES = \
	$(LUA_INCLUDE) \
	-I$(top_srcdir)/tests/catch \
	-I$(top_srcdir)/src \
	-I$(top_srcdir)/src/3rdparty
backends_LDADD = $(top_builddir)/src/libpepper.a
LIBS += \
	$(PTHREAD_LIBS) \
	$(FRAMEWORKS) \
	$(LUA_LIB)
AM_LDFLAGS = 

if SVN_BACKEND
backends_SOURCES += \
	test_subversion.h
AM_CPPFLAGS += \
	-DUSE_SUBVERSION $(APR_CPPFLAGS) $(APR_INCLUDES)
AM_CXXFLAGS += \
	-Wno-deprecated-declarations \
	$(APR_CFLAGS) \
	$(SVN_CFLAGS)
AM_LDFLAGS += \
	$(SVN_LDFLAGS) \
	$(SVN_LIBS) \
	$(APR_LIBS)
endif

if MERCURIAL_BACKEND
AM_CPPFLAGS += \
	-DUSE_MERCURIAL
AM_CXXFLAGS += \
	$(PYTHON_CPPFLAGS)
AM_LDFLAGS += \
	$(PYTHON_LDFLAGS)
endif


# Last but not least, the CFLAGS
AM_CFLAGS = $(AM_CXXFLAGS)
