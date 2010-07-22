#
# pepper - SCM statistics report generator
# Copyright (C) 2010 Jonas Gehring
#

bin_PROGRAMS = pepper

pepper_SOURCES = \
	backend.h backend.cpp \
	cache.h cache.cpp \
	main.h main.cpp \
	options.h options.cpp \
	revision.h revision.cpp \
	utils.h utils.cpp

AM_CXXFLAGS = \
	-Wall -pipe -ansi
AM_CPPFLAGS = 
AM_LDFLAGS = 


# Optional features
if GIT_BACKEND
pepper_SOURCES += \
	backends/git.h backends/git.cpp
AM_CPPFLAGS += \
	-DUSE_GIT
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
	$(SVN_LDFLAGS) 
endif
