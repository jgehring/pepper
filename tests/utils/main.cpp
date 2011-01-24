/*
 * pepper - SCM statistics report generator
 * Copyright (C) 2010-2011 Jonas Gehring
 *
 * Released under the GNU General Public License, version 3.
 * Please see the COPYING file in the source distribution for license
 * terms and conditions, or see http://www.gnu.org/licenses/.
 *
 * file: tests/utils/main.cpp
 * Short program for testing utility functions
 */


#include <cmath>
#include <cstring>
#include <iostream>
#include <string>

#include "utils.h"


// This can be used for error descriptions
static std::string errstr;


// Error string formatting
template <typename T1, typename T2>
static std::string err_invalid_return(const T1 &in, const T2 &ex, const T2 &out)
{
	std::string str = "Invalid return value for input '";
	str += in;
	str += "' (expected '" + ex + "', got '" + out + "')";
	return str;
}


// Unit testing for utils::trim()
static int test_trim()
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
		{ "\tab  \n", "ab" }
	};

	for (unsigned int i = 0; i < sizeof(inout) / sizeof(inout_t); i++) {
		std::string out = utils::trim(inout[i].in);
		if (out != inout[i].out) {
			errstr = err_invalid_return(inout[i].in, inout[i].out, out);
			return EXIT_FAILURE;
		}
	}
	return EXIT_SUCCESS;
}

// Unit testing for utils::split()
static int test_split()
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
		{ "defdef", "", std::vector<std::string>() } // ",,"
	};
	inout[2].out.push_back("1");
	inout[2].out.push_back("2");
	inout[2].out.push_back("3");
	inout[3].out.push_back("1,2,3");
	inout[4].out.push_back("");
	inout[4].out.push_back("1");
	inout[4].out.push_back("2");
	inout[4].out.push_back("3");
	inout[4].out.push_back("");
	inout[5].out.push_back("1");
	inout[5].out.push_back("2");
	inout[5].out.push_back("3");
	inout[5].out.push_back(" ");
	inout[6].out.push_back("");
	inout[6].out.push_back("");
	inout[6].out.push_back("");
	inout[7].out.push_back("d");
	inout[7].out.push_back("e");
	inout[7].out.push_back("f");
	inout[7].out.push_back("d");
	inout[7].out.push_back("e");
	inout[7].out.push_back("f");

	for (unsigned int i = 0; i < sizeof(inout) / sizeof(inout_t); i++) {
		std::vector<std::string> out = utils::split(inout[i].in, inout[i].token);
		if (out != inout[i].out) {
			std::ostringstream os;
			os << "Invalid return value for input '" << inout[i].in << "', '" << inout[i].token << "' ";
			os << "(expected '";
			for (unsigned int j = 0; j < inout[i].out.size(); j++) {
				os << inout[i].out[j];
				if (j != inout[i].out.size()-1) {
					os << ",";
				}
			}
			os << "', got '";
			for (unsigned int j = 0; j < out.size(); j++) {
				os << out[j];
				if (j != out.size()-1) {
					os << ",";
				}
			}
			os << "')";
			errstr = os.str();
			return EXIT_FAILURE;
		}
	}
	return EXIT_SUCCESS;
}

// Unit testing for utils::join()
static int test_join()
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

	for (unsigned int i = 0; i < sizeof(inout) / sizeof(inout_t); i++) {
		std::string out = utils::join(inout[i].in, inout[i].c);
		if (out != inout[i].out) {
			errstr = err_invalid_return(inout[i].c, inout[i].out, out);
			return EXIT_FAILURE;
		}

		out = utils::join(inout[i].in.begin(), inout[i].in.end(), inout[i].c);
		if (out != inout[i].out) {
			errstr = err_invalid_return(inout[i].c, inout[i].out, out);
			return EXIT_FAILURE;
		}
	}
	return EXIT_SUCCESS;
}

// Unit testing for utils::strprintf()
static int test_strprintf()
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
		{ "Longs for %ld and %lu are ok", "Longs for -23 and 100 are ok", ""}
	};
	inout[0].gen = utils::strprintf(inout[0].src);
	inout[1].gen = utils::strprintf(inout[1].src, 2, 2, 4);
	inout[2].gen = utils::strprintf(inout[2].src, 'a', "abc");
	inout[3].gen = utils::strprintf(inout[3].src, 3.14, sqrt(2.0), -1.0f, 1.0f, 3.0, -3.0);
	inout[4].gen = utils::strprintf(inout[4].src);
	inout[5].gen = utils::strprintf(inout[5].src, (long)-23, (unsigned long)100);

	for (unsigned int i = 0; i < sizeof(inout) / sizeof(inout_t); i++) {
		if (inout[i].gen != inout[i].out) {
			errstr = err_invalid_return(inout[i].src, inout[i].out, inout[i].gen);
			return EXIT_FAILURE;
		}
	}
	return EXIT_SUCCESS;
}
	
// Unit testing for utils::compress() and utils::decompress()
static int test_compress()
{
	// Test compression of small data packets and random-length ones
	const int maxlen = 256, randoms = 16;
	for (int i = 0; i < maxlen + randoms; i++) {
		std::vector<char> in(i <= maxlen ? i : (rand() & 0xFFFF));

		for (size_t j = 0; j < in.size(); j++) {
			in[j] = rand() & 0xFF;
		}
		std::vector<char> out = utils::uncompress(utils::compress(in));

		if (in.size() != out.size() || memcmp(&in[0], &out[0], in.size())) {
			std::ostringstream os;
			os << std::hex;
			os << "Invalid return value for input '";
			for (size_t j = 0; j < in.size(); j++) os << (int)in[i];
			os << "': got '";
			for (size_t j = 0; j < out.size(); j++) os << (int)out[i];
			os << "'";
			errstr = os.str();
			return EXIT_FAILURE;
		}
	}

	return EXIT_SUCCESS;
}

// Unit testing for utils::crc32()
static int test_crc32()
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

	for (unsigned int i = 0; i < sizeof(inout) / sizeof(inout_t); i++) {
		uint32_t out = utils::crc32(inout[i].in);
		if (out != inout[i].out) {
			std::ostringstream os;
			os << "Invalid return value for vector " << i;
			os << std::hex;
			os << ": expected '" << inout[i].out << "', got '" << out << "'";
			errstr = os.str();
			return EXIT_FAILURE;
		}
	}
	return EXIT_SUCCESS;
}

// Program entry point
int main(int, char **)
{
	struct part_t {
		const char *name;
		int (*function)();
	} parts[] = {
		{ "utils::trim()", test_trim },
		{ "utils::split()", test_split },
		{ "utils::join()", test_join },
		{ "utils::strprintf()", test_strprintf },
		{ "utils::compress()", test_compress },
		{ "utils::crc32()", test_crc32 }
	};

	srand(time(NULL));

	for (unsigned int i = 0; i < sizeof(parts) / sizeof(part_t); i++) {
		std::cout << "Testing " << parts[i].name << "... " << std::flush;
		try {
			if ((*parts[i].function)() == EXIT_SUCCESS) {
				std::cout << "ok" << std::endl;
			} else {
				std::cout << "failed: " << errstr << std::endl;
				return EXIT_FAILURE;
			}
		} catch (const std::exception &ex) {
			std::cout << "failed: Catched exception: " << ex.what() << std::endl;
			return EXIT_FAILURE;
		} catch (...) {
			std::cout << "failed: Catched unknown exception!" << std::endl;
			return EXIT_FAILURE;
		}
	}

	return EXIT_SUCCESS;
}
