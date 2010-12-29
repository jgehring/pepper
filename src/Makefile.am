#
# pepper - SCM statistics report generator
# Copyright (C) 2010 Jonas Gehring
#

bin_PROGRAMS = pepper

pepper_SOURCES = \
	backend.h backend.cpp \
	bstream.h bstream.cpp \
	cache.h cache.cpp \
	diffstat.h diffstat.cpp \
	globals.h globals.cpp \
	jobqueue.h \
	logger.h logger.cpp \
	luahelpers.h \
	main.h main.cpp \
	options.h options.cpp \
	report.h report.cpp \
	repository.h repository.cpp \
	revision.h revision.cpp \
	revisioniterator.h revisioniterator.cpp \
	signalhandler.h signalhandler.cpp \
	tag.h tag.cpp \
	utils.h utils.cpp \
	\
	syslib/fs.h syslib/fs.cpp \
	syslib/io.h syslib/io.cpp \
	syslib/parallel.h syslib/parallel.cpp \
	\
	3rdparty/lunar/lunar.h

AM_CXXFLAGS = \
	-Wall -W -pipe \
	$(PTHREAD_CFLAGS)
INCLUDES = \
	-I@top_srcdir@/src/3rdparty \
	-I@top_srcdir@/src/lua \
	-I@top_srcdir@/src/syslib
AM_CPPFLAGS = \
	$(LUA_INCLUDE) \
	-DDATADIR=\"$(pkgdatadir)\"
AM_LDFLAGS = \
	$(PTHREAD_LIBS) \
	$(LUA_LIB) \
	$(FRAMEWORKS)


# Optional features
if GIT_BACKEND
pepper_SOURCES += \
	backends/git.h backends/git.cpp
AM_CPPFLAGS += \
	-DUSE_GIT
endif

if MERCURIAL_BACKEND
pepper_SOURCES += \
	backends/mercurial.h backends/mercurial.cpp
AM_CPPFLAGS += \
	-DUSE_MERCURIAL
AM_CXXFLAGS += \
	$(PYTHON_CPPFLAGS)
AM_LDFLAGS += \
	$(PYTHON_LDFLAGS)
endif

if SVN_BACKEND
pepper_SOURCES += \
	backends/subversion.h backends/subversion.cpp
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

if POPEN_NOSHELL
pepper_SOURCES += \
	3rdparty/popen-noshell/popen_noshell.h 3rdparty/popen-noshell/popen_noshell.c
AM_CPPFLAGS += \
	-DUSE_POPEN_NOSHELL
endif

if GNUPLOT
pepper_SOURCES += \
	plot.h plot.cpp \
	3rdparty/gnuplot-cpp/gnuplot_i.h
AM_CPPFLAGS += \
	-DUSE_GNUPLOT
endif


# Last but not least, the CFLAGS
AM_CFLAGS = $(AM_CXXFLAGS)
