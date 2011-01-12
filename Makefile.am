#
# pepper - SCM statistics report generator
# Copyright (C) 2010 Jonas Gehring
#

SUBDIRS = src reports
if TESTSUITE
SUBDIRS += tests
endif


# Generate luadoc documentation
LUADOCS = \
	docs/lua/gnuplot.luadoc \
	docs/lua/diffstat.luadoc \
	docs/lua/report.luadoc \
	docs/lua/repository.luadoc \
	docs/lua/revision.luadoc \
	docs/lua/tag.luadoc 
luadoc: $(LUADOCS)
	luadoc -d luadoc --nofiles $(LUADOCS)
