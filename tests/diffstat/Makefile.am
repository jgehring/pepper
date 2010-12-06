#
# pepper - SCM statistics report generator
# Copyright (C) 2010 Jonas Gehring
#

bin_PROGRAMS = pdiffstat
pdiffstat_SOURCES = \
	main.cpp \
	\
	$(top_srcdir)/src/diffstat.cpp \
	$(top_srcdir)/src/utils.cpp \
	$(top_srcdir)/src/syslib/parallel.cpp

AM_CXXFLAGS = \
	-Wall -W -pipe \
	$(PTHREAD_CFLAGS)
AM_CPPFLAGS =
INCLUDES = \
	-I$(top_srcdir)/src \
	-I$(top_srcdir)/src/3rdparty
LIBS += \
	$(PTHREAD_LIBS)


# Optional features
if POPEN_NOSHELL
pdiffstat_SOURCES += \
	$(top_srcdir)/src/3rdparty/popen-noshell/popen_noshell.h \
	$(top_srcdir)/src/3rdparty/popen-noshell/popen_noshell.c
AM_CPPFLAGS += \
	-DUSE_POPEN_NOSHELL
endif


# Last but not least, the CFLAGS
AM_CFLAGS = $(AM_CXXFLAGS)
