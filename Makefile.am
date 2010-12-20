#
# pepper - SCM statistics report generator
# Copyright (C) 2010 Jonas Gehring
#

SUBDIRS = src tests reports

# Generate luadoc documentation
LUADOCS = \
	docs/lua/gnuplot.luadoc \
	docs/lua/diffstat.luadoc \
	docs/lua/report.luadoc \
	docs/lua/repository.luadoc \
	docs/lua/revision.luadoc
luadoc: $(LUADOCS)
	luadoc -d luadoc --nofiles $(LUADOCS)

# Don't include my TODO file in the distribution
dist-hook:
	rm -f "$(distdir)/TODO"
