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
	$(top_srcdir)/src/syslib/parallel.cpp \
	\
	$(top_srcdir)/src/3rdparty/popen-noshell/popen_noshell.c

AM_CXXFLAGS = \
	-Wall -W -pipe \
	$(PTHREAD_CFLAGS)
INCLUDES = \
	-I$(top_srcdir)/src \
	-I$(top_srcdir)/src/3rdparty
LIBS += \
	$(PTHREAD_LIBS)

AM_CFLAGS = $(AM_CXXFLAGS)
