#
# pepper - SCM statistics report generator
# Copyright (C) 2010 Jonas Gehring
#

SUBDIRS = src tests

# Generate luadoc documentation
LUADOCS = \
	docs/lua/diffstat.luadoc \
	docs/lua/report.luadoc \
	docs/lua/repository.luadoc \
	docs/lua/revision.luadoc
luadoc: $(LUADOCS)
	luadoc -d luadoc --nofiles $(LUADOCS)
