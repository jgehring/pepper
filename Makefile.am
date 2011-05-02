#
# pepper - SCM statistics report generator
# Copyright (C) 2010-2011 Jonas Gehring
#
# Released under the GNU General Public License, version 3.
# Please see the COPYING file in the source distribution for license
# terms and conditions, or see http://www.gnu.org/licenses/.
#

SUBDIRS = src reports
if MANPAGE
SUBDIRS += docs
endif
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
	docs/lua/iterator.luadoc \
	docs/lua/tag.luadoc \
	docs/lua/utils.luadoc \
	reports/pepper/datetime.lua \
	reports/pepper/plotutils.lua
luadoc: $(LUADOCS)
	luadoc -d luadoc --nofiles $(LUADOCS)

if MANPAGE
htmlman:
	$(MAKE) $(AM_MAKEFLAGS) -C docs htmlman
endif

DISTCHECK_CONFIGURE_FLAGS = --enable-tests
