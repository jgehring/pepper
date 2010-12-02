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
	-Wall -W -pipe -ansi \
	$(PTHREAD_CFLAGS)
INCLUDES = \
	-I$(top_srcdir)/src
LIBS += \ 
	$(PTHREAD_LIBS)
