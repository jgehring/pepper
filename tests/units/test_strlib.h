/*
 * pepper - SCM statistics report generator
 * Copyright (C) 2010-2012 Jonas Gehring
 *
 * Released under the GNU General Public License, version 3.
 * Please see the COPYING file in the source distribution for license
 * terms and conditions, or see http://www.gnu.org/licenses/.
 *
 * file: tests/units/test_strlib.h
 * Unit tests for string functions
 */


#ifndef TEST_STRLIB_H
#define TEST_STRLIB_H


#include "strlib.h"


namespace test_str
{

TEST_CASE("str/trim", "str::trim()")
{
	struct inout_t {
		std::string in;
		std::string out;
	} inout[] = {
		{ "", "" },
		{ "0", "0" },
		{ " ab", "ab" },
		{ "ab  ", "ab" },
		{ " ab ", "ab" },
		{ "\tab  \n", "ab" },
		{ "   \t", "" }
	};

	for (unsigned int i = 0; i < NUM_INOUTS; i++) {
		std::string out = str::trim(inout[i].in);
		INFO("input = \"" << inout[i].in << "\"");
		REQUIRE(out == inout[i].out);
	}
}

TEST_CASE("str/split", "str::split()")
{
	struct inout_t {
		std::string in;
		std::string token;
		std::vector<std::string> out;
	} inout[] = {
		{ "", "", std::vector<std::string>() },
		{ "", "token", std::vector<std::string>() },
		{ "1,2,3", ",", std::vector<std::string>() }, // "1,2,3"
		{ "1,2,3", "token", std::vector<std::string>() }, // "1,2,3"
		{ "abc1abc2abc3abc", "abc", std::vector<std::string>() }, // ",1,2,3,"
		{ "1abc2abc3abc ", "abc", std::vector<std::string>() }, // "1,2,3, "
		{ "defdef", "def", std::vector<std::string>() }, // ",,"
		{ "defdef", "", std::vector<std::string>() } // "d,e,f,d,e,f"
	};
	tutils::push_back(&inout[0].out, "");
	tutils::push_back(&inout[1].out, "");
	tutils::push_back(&inout[2].out, "1", "2", "3");
	tutils::push_back(&inout[3].out, "1,2,3");
	tutils::push_back(&inout[4].out, "", "1", "2", "3", "");
	tutils::push_back(&inout[5].out, "1", "2", "3", " ");
	tutils::push_back(&inout[6].out, "", "", "");
	tutils::push_back(&inout[7].out, "d", "e", "f", "d", "e", "f");

	for (unsigned int i = 0; i < NUM_INOUTS; i++) {
		std::vector<std::string> out = str::split(inout[i].in, inout[i].token);
		INFO("input = \"" << inout[i].in << "\"");
		REQUIRE(out == inout[i].out);
	}
}

TEST_CASE("str/join", "str::join()")
{
	struct inout_t {
		std::vector<std::string> in;
		std::string c;
		std::string out;
	} inout[] = {
		{ std::vector<std::string>(2, "a"), "b", "aba"},
		{ std::vector<std::string>(3, "ab"), "c", "abcabcab"},
		{ std::vector<std::string>(3, "a"), "bc", "abcabca"},
		{ std::vector<std::string>(), "c", "" }
	};

	for (unsigned int i = 0; i < NUM_INOUTS; i++) {
		std::string out = str::join(inout[i].in, inout[i].c);
		REQUIRE(out == inout[i].out);

		out = str::join(inout[i].in.begin(), inout[i].in.end(), inout[i].c);
		REQUIRE(out == inout[i].out);
	}
}

TEST_CASE("str/startsWith", "str::startsWith()")
{
	REQUIRE(str::startsWith("test", "te") == true);
	REQUIRE(str::startsWith("test", "fe") == false);
	REQUIRE(str::startsWith("test", "test") == true);
	REQUIRE(str::startsWith("test", "testing") == false);
	REQUIRE(str::startsWith(std::string(), std::string()) == true);
	REQUIRE(str::startsWith(std::string(), "test") == false);
	REQUIRE(str::startsWith("test", std::string()) == true);
}

TEST_CASE("str/endsWith", "str::endsWith()")
{
	REQUIRE(str::endsWith("test", "st") == true);
	REQUIRE(str::endsWith("test", "at") == false);
	REQUIRE(str::endsWith("test", "test") == true);
	REQUIRE(str::endsWith("test", "atest") == false);
	REQUIRE(str::endsWith(std::string(), std::string()) == true);
	REQUIRE(str::endsWith(std::string(), "test") == false);
	REQUIRE(str::endsWith("test", std::string()) == true);
}

TEST_CASE("str/printf", "str::printf()")
{
	struct inout_t {
		const char *src;
		std::string out;
		std::string gen;
	} inout[] = {
		{ "Hello, world", "Hello, world", "" },
		{ "Ints: %d + %i = %u", "Ints: 2 + 2 = 4", "" },
		{ "Strings: %c and %s", "Strings: a and abc", "" },
		{ "Floats: %e, %E, %f, %F, %g, %G", "Floats: 3.14, 1.41421, -1, 1, 3, -3", ""},
		{ "Escaping %% works", "Escaping % works", ""},
		{ "Longs for %ld and %lu are ok", "Longs for -23 and 100 are ok", ""},
		{ "64bit integers are working: %lld and %llu", "64bit integers are working: -4294967296 and 4294967296", ""}
	};
	inout[0].gen = str::printf(inout[0].src);
	REQUIRE(inout[0].gen == inout[0].out);
	inout[1].gen = str::printf(inout[1].src, 2, 2, 4);
	REQUIRE(inout[1].gen == inout[1].out);
	inout[2].gen = str::printf(inout[2].src, 'a', "abc");
	REQUIRE(inout[2].gen == inout[2].out);
	inout[3].gen = str::printf(inout[3].src, 3.14, sqrt(2.0), -1.0f, 1.0f, 3.0, -3.0);
	REQUIRE(inout[3].gen == inout[3].out);
	inout[4].gen = str::printf(inout[4].src);
	REQUIRE(inout[4].gen == inout[4].out);
	inout[5].gen = str::printf(inout[5].src, (long)-23, (unsigned long)100);
	REQUIRE(inout[5].gen == inout[5].out);
	inout[6].gen = str::printf(inout[6].src, -4294967296LL, 4294967296ULL);
	REQUIRE(inout[6].gen == inout[6].out);
}

} // namespace test_str

#endif // TEST_STRLIB_H
