#
# pepper - SCM statistics report generator
# Copyright (C) 2010-2014 Jonas Gehring
#
# Released under the GNU General Public License, version 3.
# Please see the COPYING file in the source distribution for license
# terms and conditions, or see http://www.gnu.org/licenses/.
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

SUBDIRS = diffstat units backends

EXTRA_DIST = \
	testsuite.at \
	$(srcdir)/package.m4 \
	$(TESTSUITE) \
	atlocal.in \
	\
	test_diffstat.at \
	test_diffstat.pl \
	test_units.at \
	test_backends.at \
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
	diffstat/data/svn-mod.ref \
	\
	catch/catch.hpp \
	catch/catch_reporter_basic.hpp \
	catch/catch_reporter_junit.hpp \
	catch/catch_reporter_xml.hpp \
	catch/catch_runner.hpp \
	catch/catch_with_main.hpp \
	catch/LICENSE_1_0.txt \
	catch/internal/catch_common.h \
	catch/internal/catch_hub.h \
	catch/internal/catch_interfaces_capture.h \
	catch/internal/catch_interfaces_reporter.h \
	catch/internal/catch_interfaces_runner.h \
	catch/internal/catch_interfaces_testcase.h \
	catch/internal/catch_result_type.h \
	catch/internal/catch_capture.hpp \
	catch/internal/catch_commandline.hpp \
	catch/internal/catch_config.hpp \
	catch/internal/catch_debugger.hpp \
	catch/internal/catch_generators.hpp \
	catch/internal/catch_generators_impl.hpp \
	catch/internal/catch_hub_impl.hpp \
	catch/internal/catch_list.hpp \
	catch/internal/catch_reporter_registrars.hpp \
	catch/internal/catch_reporter_registry.hpp \
	catch/internal/catch_resultinfo.hpp \
	catch/internal/catch_runner_impl.hpp \
	catch/internal/catch_section.hpp \
	catch/internal/catch_self_test.hpp \
	catch/internal/catch_stream.hpp \
	catch/internal/catch_test_case_info.hpp \
	catch/internal/catch_test_case_registry_impl.hpp \
	catch/internal/catch_test_registry.hpp \
	catch/internal/catch_xmlwriter.hpp

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
