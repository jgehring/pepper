/*
 * pepper - SCM statistics report generator
 * Copyright (C) 2010-2011 Jonas Gehring
 *
 * Released under the GNU General Public License, version 3.
 * Please see the COPYING file in the source distribution for license
 * terms and conditions, or see http://www.gnu.org/licenses/.
 *
 * file: tests/units/test_options.h
 * Unit tests for the Options class
 */


#ifndef TEST_OPTIONS_H
#define TEST_OPTIONS_H


#include <cstdarg>

#include "syslib/fs.h"

#include "options.h"


namespace test_options
{

struct data_t
{
	char **args;
	int nargs;
	stringmap options;
	stringmap reportOptions;

	data_t() { }
	data_t(const Options &defaults) {
		options = defaults.m_options;
		reportOptions = defaults.m_reportOptions;
	}

	std::string cmdline() {
		std::ostringstream oss;
		for (int i = 0; i < nargs; i++) {
			oss << args[i];
			if (i < nargs-1) oss << " ";
		}
		return oss.str();
	}

	void setupArgs(int n, ...)
	{
		args = new char*[n+2];
		args[0] = (char *)"pepper";
		va_list vl;
		va_start(vl, n);
		for (int i = 0; i < n; i++) {
			args[i+1] = va_arg(vl, char *);
		}
		args[n+1] = NULL;
		va_end(vl);
		nargs = n+1;
	}
};

TEST_CASE("options/cmdlines", "Command line parsing")
{
	using test_options::data_t;

	std::vector<data_t> tests;
	Options defaults;

	data_t help1(defaults);
	help1.setupArgs(1, "-h");
	help1.options["help"] = "true";
	tests.push_back(help1);

	data_t help2(defaults);
	help2.setupArgs(1, "--help");
	help2.options["help"] = "true";
	tests.push_back(help2);

	data_t help3(defaults);
	help3.setupArgs(1, "-?");
	help3.options["help"] = "true";
	tests.push_back(help3);

	data_t version(defaults);
	version.setupArgs(1, "--version");
	version.options["version"] = "true";
	tests.push_back(version);

	data_t simple(defaults);
	simple.setupArgs(2, "loc", "http://svn.example.org");
	simple.options["report"] = "loc";
	simple.options["repository"] = "http://svn.example.org";
	tests.push_back(simple);

	data_t report(defaults);
	report.setupArgs(4, "loc", "--branch=trunk", "-tpng", "http://svn.example.org");
	report.options["report"] = "loc";
	report.options["repository"] = "http://svn.example.org";
	report.reportOptions["branch"] = "trunk";
	report.reportOptions["t"] = "png";
	tests.push_back(report);

	data_t backend(defaults);
	backend.setupArgs(4, "-bsvn", "authors", "-tpng", "http://svn.example.org");
	backend.options["backend"] = "svn";
	backend.options["report"] = "authors";
	backend.options["repository"] = "http://svn.example.org";
	backend.reportOptions["t"] = "png";
	tests.push_back(backend);

	data_t backend2(defaults);
	backend2.setupArgs(6, "--backend=svn", "--username=test", "--non-interactive", "authors", "-tpng", "http://svn.example.org");
	backend2.options["backend"] = "svn";
	backend2.options["report"] = "authors";
	backend2.options["repository"] = "http://svn.example.org";
	backend2.reportOptions["t"] = "png";
	backend2.options["username"] = "test";
	backend2.options["non-interactive"] = std::string();
	tests.push_back(backend2);

	data_t full(defaults);
	full.setupArgs(8, "-v", "--no-cache", "-bsvn", "--username=test", "--non-interactive", "authors", "-tpng", "http://svn.example.org");
	full.options["backend"] = "svn";
	full.options["cache"] = "false";
	full.options["report"] = "authors";
	full.options["repository"] = "http://svn.example.org";
	full.reportOptions["t"] = "png";
	full.options["username"] = "test";
	full.options["non-interactive"] = std::string();
	tests.push_back(full);

	data_t bhelp(defaults);
	bhelp.setupArgs(2, "-bsvn", "-h");
	bhelp.options["backend"] = "svn";
	bhelp.options["help"] = "true";
	tests.push_back(bhelp);

	data_t norepo(defaults);
	norepo.setupArgs(1, "loc");
	norepo.options["report"] = "loc";
	norepo.options["repository"] = sys::fs::cwd();
	tests.push_back(norepo);

	data_t rhelp(defaults);
	rhelp.setupArgs(2, "loc", "-h");
	rhelp.options["report"] = "loc";
	rhelp.options["repository"] = sys::fs::cwd();
	rhelp.options["help"] = "true";
	tests.push_back(rhelp);

	data_t rhelp2(defaults);
	rhelp2.setupArgs(4, "-bsvn", "loc", "http://svn.example.org", "-h");
	rhelp2.options["backend"] = "svn";
	rhelp2.options["report"] = "loc";
	rhelp2.options["repository"] = "http://svn.example.org";
	rhelp2.options["help"] = "true";
	tests.push_back(rhelp2);

	// Run tests
	for (std::vector<data_t>::size_type i = 0;  i < tests.size(); i++) {
		Options opts;
		opts.parse(tests[i].nargs, tests[i].args);
		INFO(tests[i].cmdline());
		REQUIRE(opts.m_options == tests[i].options);
		REQUIRE(opts.m_reportOptions == tests[i].reportOptions);
	}
}

} // namespace test_options

#endif // TEST_OPTIONS_H
