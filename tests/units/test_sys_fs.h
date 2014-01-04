/*
 * pepper - SCM statistics report generator
 * Copyright (C) 2010-2014 Jonas Gehring
 *
 * Released under the GNU General Public License, version 3.
 * Please see the COPYING file in the source distribution for license
 * terms and conditions, or see http://www.gnu.org/licenses/.
 *
 * file: tests/units/test_sys_fs.h
 * Unit tests for file system utility functions
 */


#ifndef TEST_SYS_FS_H
#define TEST_SYS_FS_H


#include "syslib/fs.h"


namespace test_sys_fs
{

TEST_CASE("sys_fs/basename", "sys::fs::basename()")
{
	struct inout_t {
		const char *in;
		std::string out;
	} inout[] = {
		{ "", "" },
		{ ".", "." },
		{ "..", ".." },
		{ "../.", "." },
		{ "/", "/" },
		{ "../../ab", "ab" },
		{ "/ab/cd/ef/gh.ij", "gh.ij" },
		{ "ab/cd", "cd" },
		{ "/absolute", "absolute" },
		{ "relative", "relative" },
		{ "./rel", "rel" },
		{ "/ab/cd/ef/", "ef" },
		{ "reldir/", "reldir" }
	};

	for (unsigned int i = 0; i < NUM_INOUTS; i++) {
		std::string out = sys::fs::basename(inout[i].in);
		REQUIRE(out == inout[i].out);
	}
}

TEST_CASE("sys_fs/dirname", "sys::fs::dirname()")
{
	struct inout_t {
		const char *in;
		std::string out;
	} inout[] = {
		{ "", "." },
		{ ".", "." },
		{ "..", "." },
		{ "../.", ".." },
		{ "/", "/" },
		{ "../../ab", "../.." },
		{ "/ab/cd/ef/gh.ij", "/ab/cd/ef" },
		{ "ab/cd", "ab" },
		{ "/absolute", "/" },
		{ "relative", "." },
		{ "./rel", "." },
		{ "/ab/cd/ef/", "/ab/cd" },
		{ "reldir/", "." }
	};

	for (unsigned int i = 0; i < NUM_INOUTS; i++) {
		std::string out = sys::fs::dirname(inout[i].in);
		REQUIRE(out == inout[i].out);
	}
}

} // namespace test_sys_fs

#endif // TEST_SYS_FS_H
