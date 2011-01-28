/*
 * pepper - SCM statistics report generator
 * Copyright (C) 2010-2011 Jonas Gehring
 *
 * Released under the GNU General Public License, version 3.
 * Please see the COPYING file in the source distribution for license
 * terms and conditions, or see http://www.gnu.org/licenses/.
 *
 * file: tests/bstreams/main.cpp
 * Short program for testing binary streams functions
 */


#include <cstdlib>
#include <cstring>
#include <iostream>
#include <sstream>

#include "main.h"

#include "bstream.h"


// This can be used for error descriptions
static std::string errstr;


static int test_rw()
{
	for (int i = 1; i < 4097; i++) {
		char *in = new char[i];
		int s = rand();
		for (int j = 0; j < i; j++) {
			in[j] = (i+s) & 0xFF;
		}

		char *out = new char[i];

		MOStream sout;
		sout.write(out, i);
		MIStream sin(sout.data());
		sin.read(in, i);
		if (memcmp(in, out, i)) {
			return EXIT_FAILURE;
		}

		delete[] out;
		delete[] in;
	}

	return EXIT_SUCCESS;
}

static int test_ops()
{
	// Characters
	MOStream outc;
	for (int i = 0; i < 127; i++) {
		outc << char(i);
	}
	MIStream inc(outc.data());
	for (int i = 0; i < 127; i++) {
		char c; inc >> c;
		if (int(c) != i) {
			std::ostringstream os;
			os << "Unable to read character with code " << i;
			errstr = os.str();
			return EXIT_FAILURE;
		}
	}

	// Integers - 32 bit
	MOStream out32;
	for (int i = 0; i < 1024; i++) {
		out32 << (uint32_t)i;
	}
	MIStream in32(out32.data());
	for (int i = 0; i < 1024; i++) {
		uint32_t c; in32 >> c;
		if (c != (uint32_t)i) {
			std::ostringstream os;
			os << "Unable to read uint32_t " << i;
			errstr = os.str();
			return EXIT_FAILURE;
		}
	}

	// Integers - 64 bit
	MOStream out64;
	for (int i = -1024; i < 1024; i++) {
		out64 << (int64_t)i;
	}
	MIStream in64(out64.data());
	for (int i = -1024; i < 1024; i++) {
		int64_t c; in64 >> c;
		if (c != i) {
			std::ostringstream os;
			os << "Unable to read int64_t " << i;
			errstr = os.str();
			return EXIT_FAILURE;
		}
	}

	// Integers - 64 bit
	MOStream outu64;
	for (int i = 0; i < 1024; i++) {
		outu64 << (uint64_t)i;
	}
	MIStream inu64(outu64.data());
	for (int i = 0; i < 1024; i++) {
		uint64_t c; inu64 >> c;
		if (c != (uint64_t)i) {
			std::ostringstream os;
			os << "Unable to read uint64_t " << i;
			errstr = os.str();
			return EXIT_FAILURE;
		}
	}

	// String testing
	std::string s;
	for (int i = 1; i < 1025; i++) {
		s.clear(); s.resize(i);
		for (size_t j = 0; j < s.size(); j++) {
			s[j] = 32 + (rand() % 94);
		}

		MOStream outs;
		int n = 1 + (rand() % 10);
		for (int j = 0; j < n; j++) {
			outs << s;
		}

		MIStream ins(outs.data());
		std::string d;
		for (int j = 0; j < n; j++) {
			ins >> d;
			if (d != s) {
				std::ostringstream os;
				os << "Unable to read: " << j << " of " << n << " times a string with length " << s.size();
				os << " (" << d << " != " << s << ")";
				errstr = os.str();
				return EXIT_FAILURE;
			}
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
		{ "bstream read/write", test_rw },
		{ "bstream operators", test_ops }
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
