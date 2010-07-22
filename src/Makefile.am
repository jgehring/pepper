#
# pepper - SCM statistics report generator
# Copyright (C) 2010 Jonas Gehring
#

bin_PROGRAMS = pepper

pepper_SOURCES = \
	backend.h backend.cpp \
	main.h main.cpp \
	options.h options.cpp \
	utils.h utils.cpp

AM_CXXFLAGS = \
	-Wall -pipe -ansi
AM_CPPFLAGS = 
AM_LDFLAGS = 


# Optional features
if GIT_BACKEND
AM_CPPFLAGS += \
	-DUSE_GIT
endif

if SVN_BACKEND
AM_CPPFLAGS += \
	-DUSE_SUBVERSION $(APR_CPPFLAGS) $(APR_INCLUDES)
AM_CXXFLAGS += \
	$(APR_CFLAGS) \
	$(SVN_CFLAGS)
AM_LDFLAGS += \
	$(APR_LIBS) \
	$(SVN_LDFLAGS) 
endif
