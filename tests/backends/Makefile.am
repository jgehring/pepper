#
# pepper - SCM statistics report generator
# Copyright (C) 2010-2011 Jonas Gehring
#
# Released under the GNU General Public License, version 3.
# Please see the COPYING file in the source distribution for license
# terms and conditions, or see http://www.gnu.org/licenses/.
#

noinst_PROGRAMS = backends
backends_SOURCES = \
	main.cpp \
	\
	$(top_srcdir)/src/backend.cpp \
	$(top_srcdir)/src/bstream.cpp \
	$(top_srcdir)/src/cache.cpp \
	$(top_srcdir)/src/diffstat.cpp \
	$(top_srcdir)/src/logger.cpp \
	$(top_srcdir)/src/options.cpp \
	$(top_srcdir)/src/pex.cpp \
	$(top_srcdir)/src/revision.cpp \
	$(top_srcdir)/src/tag.cpp \
	$(top_srcdir)/src/utils.cpp \
	\
	$(top_srcdir)/src/syslib/fs.cpp \
	$(top_srcdir)/src/syslib/datetime.cpp \
	$(top_srcdir)/src/syslib/sigblock.cpp \
	$(top_srcdir)/src/syslib/parallel.cpp 

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
AM_LDFLAGS = \
	$(PTHREAD_LIBS) \
	$(LUA_LIB) \
	$(FRAMEWORKS)

if SVN_BACKEND
backends_SOURCES += \
	test_subversion.h \
	$(top_srcdir)/src/backends/subversion.h \
	$(top_srcdir)/src/backends/subversion.cpp \
	$(top_srcdir)/src/backends/subversion_p.h \
	$(top_srcdir)/src/backends/subversion_delta.cpp
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


# Last but not least, the CFLAGS
AM_CFLAGS = $(AM_CXXFLAGS)
