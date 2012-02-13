/*
 * pepper - SCM statistics report generator
 * Copyright (C) 2010-2012 Jonas Gehring
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
