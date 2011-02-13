/*
 * pepper - SCM statistics report generator
 * Copyright (C) 2010-2011 Jonas Gehring
 *
 * Released under the GNU General Public License, version 3.
 * Please see the COPYING file in the source distribution for license
 * terms and conditions, or see http://www.gnu.org/licenses/.
 *
 * file: tests/units/test_utils.h
 * Unit tests for utility functions
 */


#ifndef TEST_UTILS_H
#define TEST_UTILS_H


#include "utils.h"


namespace test_utils
{

TEST_CASE("utils/trim", "utils::trim()")
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
		std::string out = utils::trim(inout[i].in);
		INFO("input = \"" << inout[i].in << "\"");
		REQUIRE(out == inout[i].out);
	}
}

TEST_CASE("utils/split", "utils::split()")
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
		std::vector<std::string> out = utils::split(inout[i].in, inout[i].token);
		INFO("input = \"" << inout[i].in << "\"");
		REQUIRE(out == inout[i].out);
	}
}

TEST_CASE("utils/join", "utils::join()")
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
		std::string out = utils::join(inout[i].in, inout[i].c);
		REQUIRE(out == inout[i].out);

		out = utils::join(inout[i].in.begin(), inout[i].in.end(), inout[i].c);
		REQUIRE(out == inout[i].out);
	}
}

TEST_CASE("utils/strprintf", "utils::strprintf()")
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
	inout[0].gen = utils::strprintf(inout[0].src);
	REQUIRE(inout[0].gen == inout[0].out);
	inout[1].gen = utils::strprintf(inout[1].src, 2, 2, 4);
	REQUIRE(inout[1].gen == inout[1].out);
	inout[2].gen = utils::strprintf(inout[2].src, 'a', "abc");
	REQUIRE(inout[2].gen == inout[2].out);
	inout[3].gen = utils::strprintf(inout[3].src, 3.14, sqrt(2.0), -1.0f, 1.0f, 3.0, -3.0);
	REQUIRE(inout[3].gen == inout[3].out);
	inout[4].gen = utils::strprintf(inout[4].src);
	REQUIRE(inout[4].gen == inout[4].out);
	inout[5].gen = utils::strprintf(inout[5].src, (long)-23, (unsigned long)100);
	REQUIRE(inout[5].gen == inout[5].out);
	inout[6].gen = utils::strprintf(inout[6].src, -4294967296LL, 4294967296ULL);
	REQUIRE(inout[6].gen == inout[6].out);
}

TEST_CASE("utils/compress", "utils::compress() and utils::uncompress()")
{
	// Test compression of small data packets and random-length ones
	const int maxlen = 256, randoms = 16;
	for (int i = 0; i < maxlen + randoms; i++) {
		std::vector<char> in(i <= maxlen ? i : (rand() & 0xFFFF));
		for (size_t j = 0; j < in.size(); j++) in[j] = rand() & 0xFF;

		std::vector<char> out = utils::uncompress(utils::compress(in));
		REQUIRE(in.size() == out.size());
		REQUIRE(in == out);
	}
}

TEST_CASE("utils/crc32", "utils::crc32()")
{
	struct inout_t {
		std::vector<char> in;
		uint32_t out;
	} inout[] = {
		{ std::vector<char>(40, 0x00), 0xE9EC3DB1 },
		{ std::vector<char>(40, 0xFF), 0x8CD04C73 },
		{ std::vector<char>(40), 0x0DA62E3C }
	};
	for (int i = 0; i < 40; i++) inout[2].in[i] = i;

	for (unsigned int i = 0; i < NUM_INOUTS; i++) {
		uint32_t out = utils::crc32(inout[i].in);
		REQUIRE(out == inout[i].out);
	}
}

} // namespace test_utils

#endif // TEST_UTILS_H
