#
# pepper - SCM statistics report generator
# Copyright (C) 2010 Jonas Gehring
#

# The `:;' works around a Bash 3.2 bug when the output is not writeable.
$(srcdir)/package.m4: $(top_srcdir)/configure.ac
	:;{ \
		echo '# Signature of the current package.' && \
		echo 'm4_define([AT_PACKAGE_NAME],' && \
		echo '  [@PACKAGE_NAME@])' && \
		echo 'm4_define([AT_PACKAGE_TARNAME],' && \
		echo '  [@PACKAGE_TARNAME@])' && \
		echo 'm4_define([AT_PACKAGE_VERSION],' && \
		echo '  [@PACKAGE_VERSION@])' && \
		echo 'm4_define([AT_PACKAGE_STRING],' && \
		echo '  [@PACKAGE_STRING@])' && \
		echo 'm4_define([AT_PACKAGE_BUGREPORT],' && \
		echo '  [@PACKAGE_BUGREPORT@])'; \
		echo 'm4_define([AT_PACKAGE_URL],' && \
		echo '  [@PACKAGE_URL@])'; \
	} >'$(srcdir)/package.m4'

SUBDIRS = diffstat options utils bstreams

EXTRA_DIST = \
	testsuite.at \
	$(srcdir)/package.m4 \
	$(TESTSUITE) \
	atlocal.in \
	\
	test_diffstat.at \
	test_diffstat.pl \
	test_options.at \
	test_utils.at \
	\
	diffstat/data/git-add.pat \
	diffstat/data/git-add.ref \
	diffstat/data/git-del.pat \
	diffstat/data/git-del.ref \
	diffstat/data/git-mod.pat \
	diffstat/data/git-mod.ref \
	diffstat/data/git-quotes.pat \
	diffstat/data/git-quotes.ref \
	diffstat/data/svn-add.pat \
	diffstat/data/svn-add.ref \
	diffstat/data/svn-del.pat \
	diffstat/data/svn-del.ref \
	diffstat/data/svn-mod.pat \
	diffstat/data/svn-mod.ref

TESTSUITE = $(srcdir)/testsuite

DISTCLEANFILES = atconfig

check-local: atconfig atlocal $(TESTSUITE)
	$(SHELL) '$(TESTSUITE)' $(TESTSUITEFLAGS)

installcheck-local: atconfig atlocal $(TESTSUITE)
	$(SHELL) '$(TESTSUITE)' AUTOTEST_PATH='$(bindir)' \
		$(TESTSUITEFLAGS)

clean-local:
	test ! -f '$(TESTSUITE)' || \
		$(SHELL) '$(TESTSUITE)' --clean

AUTOM4TE = $(SHELL) $(top_srcdir)/missing --run autom4te
AUTOTEST = $(AUTOM4TE) --language=autotest
$(TESTSUITE): $(srcdir)/testsuite.at $(srcdir)/package.m4
	$(AUTOTEST) -I '$(srcdir)' -o $@.tmp $@.at
	mv $@.tmp $@
