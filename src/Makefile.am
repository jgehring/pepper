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
	main.h main.cpp \
	options.h options.cpp \
	repository.h repository.cpp \
	revision.h revision.cpp \
	revisioniterator.h revisioniterator.cpp \
	utils.h utils.cpp \
	\
	syslib/fs.h syslib/fs.cpp \
	syslib/parallel.h syslib/parallel.cpp \
	\
	lua/luadiffstat.h \
	lua/luahelpers.h \
	lua/luarepository.h \
	lua/luarevision.h \
	lua/plot.h lua/plot.cpp \
	lua/report.h lua/report.cpp \
	\
	3rdparty/gnuplot-cpp/gnuplot_i.h \
	3rdparty/lunar/lunar.h

AM_CXXFLAGS = \
	-Wall -pipe -ansi \
	$(LUA_CFLAGS)
INCLUDES = \
	-I@top_srcdir@/src/3rdparty \
	-I@top_srcdir@/src/lua \
	-I@top_srcdir@/src/syslib
AM_CPPFLAGS = 
AM_LDFLAGS = \
	$(LUA_LIBS)


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
	$(APR_CFLAGS) \
	$(SVN_CFLAGS)
AM_LDFLAGS += \
	$(APR_LIBS) \
	$(SVN_LDFLAGS) \
	$(SVN_LIBS) 
endif
