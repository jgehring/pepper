#
# pepper - SCM statistics report generator
# Copyright (C) 2010-2011 Jonas Gehring
#
# Released under the GNU General Public License, version 3.
# Please see the COPYING file in the source distribution for license
# terms and conditions, or see http://www.gnu.org/licenses/.
#

# Pack all code into a static library, so the unit test programs
# can use it easily.
noinst_LIBRARIES = libpepper.a
bin_PROGRAMS = pepper

libpepper_a_SOURCES = \
	abstractcache.h abstractcache.cpp \
	backend.h backend.cpp \
	bstream.h bstream.cpp \
	cache.h cache.cpp \
	diffstat.h diffstat.cpp \
	jobqueue.h \
	logger.h logger.cpp \
	luahelpers.h \
	luamodules.h luamodules.cpp \
	main.h \
	options.h options.cpp \
	pex.h pex.cpp \
	report.h report.cpp \
	repository.h repository.cpp \
	revision.h revision.cpp \
	revisioniterator.h revisioniterator.cpp \
	strlib.h strlib.cpp \
	tag.h tag.cpp \
	utils.h utils.cpp \
	\
	syslib/fs.h syslib/fs.cpp \
	syslib/io.h syslib/io.cpp \
	syslib/parallel.h syslib/parallel.cpp \
	syslib/sigblock.h syslib/sigblock.cpp \
	syslib/datetime.h syslib/datetime.cpp \
	\
	3rdparty/lunar/lunar.h

# This is the main program
pepper_SOURCES = \
	main.cpp
pepper_LDADD = \
	libpepper.a

AM_CXXFLAGS = \
	-Wall -W -pipe \
	$(PTHREAD_CFLAGS)
INCLUDES = \
	-I@top_srcdir@/src/3rdparty
AM_CPPFLAGS = \
	$(LUA_INCLUDE) \
	-DDATADIR=\"$(pkgdatadir)\"
AM_LDFLAGS = \
	$(PTHREAD_LIBS) \
	$(LUA_LIB) \
	$(FRAMEWORKS)

# Debugging?
if DEBUG
AM_CXXFLAGS += \
	-rdynamic
AM_LDFLAGS += \
	-rdynamic
endif


# Optional features
if GIT_BACKEND
libpepper_a_SOURCES += \
	backends/git.h backends/git.cpp
AM_CPPFLAGS += \
	-DUSE_GIT
endif

if MERCURIAL_BACKEND
libpepper_a_SOURCES += \
	backends/mercurial.h backends/mercurial.cpp
AM_CPPFLAGS += \
	-DUSE_MERCURIAL
AM_CXXFLAGS += \
	$(PYTHON_CPPFLAGS)
AM_LDFLAGS += \
	$(PYTHON_LDFLAGS)
endif

if SVN_BACKEND
libpepper_a_SOURCES += \
	backends/subversion.h backends/subversion.cpp \
	backends/subversion_p.h \
	backends/subversion_delta.cpp
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

if GNUPLOT
libpepper_a_SOURCES += \
	plot.h plot.cpp \
	gnuplot.h gnuplot.cpp
AM_CPPFLAGS += \
	-DUSE_GNUPLOT
endif

if LEVELDB
libpepper_a_SOURCES += \
	ldbcache.h ldbcache.cpp
AM_CXXFLAGS += \
	$(LEVELDB_CPPFLAGS)
LIBS += \
	$(LEVELDB_LIBS)
AM_CPPFLAGS += \
	-DUSE_LDBCACHE
endif


# Last but not least, the CFLAGS
AM_CFLAGS = $(AM_CXXFLAGS)
