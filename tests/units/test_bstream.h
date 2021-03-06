/*
 * pepper - SCM statistics report generator
 * Copyright (C) 2010-present Jonas Gehring
 *
 * Released under the GNU General Public License, version 3.
 * Please see the COPYING file in the source distribution for license
 * terms and conditions, or see http://www.gnu.org/licenses/.
 *
 * file: tests/units/test_bstream.h
 * Unit tests for BStream classes
 */


#ifndef TEST_BSTREAM_H
#define TEST_BSTREAM_H


#include "bstream.h"


namespace test_bstream
{

TEST_CASE("bstream/readwrite", "Reading and writing with memory streams using read() and write()")
{
	std::vector<char> in, out;
	int n = 258;
	for (int i = 0; i < n; i++) {
		in.resize(i);
		out.resize(i);

		int s = rand();
		for (int j = 0; j < i; j++) {
			out[j] = (i+s) & 0xFF;
		}

		int t = n-i;
		MOStream sout;
		for (int j = 0; j < t; j++) {
			sout.write(&(out[0]), i);
		}
		MIStream sin(sout.data());
		for (int j = 0; j < t; j++) {
			sin.read(&(in[0]), i);
			REQUIRE(!memcmp(&(in[0]), &(out[0]), i));
		}
	}
}

TEST_CASE("bstream/rw_large", "Reading and writing of large blocks")
{
	std::vector<char> in, out;
	int64_t max = 1024*1024*10;
	for (int64_t i = 1; i < max; i *= 2) {
		in.resize(i);
		out.resize(i);
		MOStream sout;
		sout.write(&(out[0]), i);
		MIStream sin(sout.data());
		sin.read(&(in[0]), i);
		INFO("Testing with " << i << " bytes");
		REQUIRE(!memcmp(&(in[0]), &(out[0]), i));
	}
}

TEST_CASE("bstream/seek", "Seek and tell for memory streams")
{
	std::vector<char> in(10), out(1);
	MOStream sout;
	sout.write(&(out[0]), out.size());
	MIStream sin(sout.data());

	REQUIRE(sin.seek(0) == true);
	REQUIRE(sin.seek(100) == false);
	REQUIRE(sin.seek(1) == true);
	REQUIRE(sin.seek(0) == true);
	REQUIRE(sout.seek(100) == false);
	REQUIRE(sout.seek(0) == true);

	out.resize(400);
	sout.write(&(out[0]), out.size());
	REQUIRE(sout.tell() == out.size());
	sout.write(&(out[0]), out.size());
	REQUIRE(sout.tell() == 2*out.size());
}

TEST_CASE("bstream/checks", "Corner cases for memory streams")
{
	std::vector<char> in(10), out(1);
	MOStream sout;
	sout.write(&(out[0]), out.size());
	MIStream sin(sout.data());
	ssize_t n = sin.read(&(in[0]), in.size());
	REQUIRE(n == ssize_t(out.size()));
}

TEST_CASE("bstream/operators", "Stream operators")
{
	SECTION("Characters", "char operators")
	{
		MOStream outc;
		for (int i = 0; i < 127; i++) {
			outc << char(i);
		}
		MIStream inc(outc.data());
		for (int i = 0; i < 127; i++) {
			char c; inc >> c;
			REQUIRE(int(c) == i);
		}
	}

	SECTION("Integers (32bit)", "uint32_t operators")
	{
		MOStream out32;
		for (uint32_t i = 0; i < 1024; i++) {
			out32 << i;
		}
		MIStream in32(out32.data());
		for (uint32_t i = 0; i < 1024; i++) {
			uint32_t c; in32 >> c;
			REQUIRE(c == i);
		}
	}

	SECTION("Integers (64bit)", "uint64_t operators")
	{
		MOStream out64;
		for (int i = -1024; i < 1024; i++) {
			out64 << (int64_t)i;
		}
		MIStream in64(out64.data());
		for (int i = -1024; i < 1024; i++) {
			int64_t c; in64 >> c;
			REQUIRE(c == i);
		}
	}

	SECTION("Integers (64bit)", "int64_t operators")
	{
		MOStream outu64;
		for (int i = 0; i < 1024; i++) {
			outu64 << (uint64_t)i;
		}
		MIStream inu64(outu64.data());
		for (int i = 0; i < 1024; i++) {
			uint64_t c; inu64 >> c;
			REQUIRE(c == (uint64_t)i);
		}
	}

	SECTION("Strings", "std::string operators")
	{
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
				REQUIRE(d == s);
			}
		}
	}
}

} // namespace test_bstream

#endif // TEST_BSTREAM_H
