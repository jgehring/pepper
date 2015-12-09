/*
 * pepper - SCM statistics report generator
 * Copyright (C) 2010-present Jonas Gehring
 *
 * Released under the GNU General Public License, version 3.
 * Please see the COPYING file in the source distribution for license
 * terms and conditions, or see http://www.gnu.org/licenses/.
 *
 * file: tests/units/test_sys_io.h
 * Unit tests for I/O utility functions
 */


#ifndef TEST_SYS_IO_H
#define TEST_SYS_IO_H


#include "syslib/io.h"
#include "syslib/parallel.h"


namespace test_sys_io
{

class StreambufReader : public sys::parallel::Thread
{
public:
	StreambufReader(sys::io::PopenStreambuf *buf)
		: m_buf(buf) {
	}

	std::string str() const {
		return m_out.str();
	}

	sys::io::PopenStreambuf *m_buf;
	std::ostringstream m_out;

protected:
	void run() {
		char buffer[1024];
		std::istream in(m_buf);
		while (in.good()) {
			in.read(buffer, sizeof(buffer));
			m_out.write(buffer, in.gcount());
		}
	}
};

TEST_CASE("sys_io/popenstreambuf", "sys::io::PopenStreambuf")
{
	int strlens[] = {0, 1, 2, 3, 7, 8, 9, 64, 1024, 2048, 4096, 4097, 1<<16};
	for (unsigned int i = 0; i < sizeof(strlens) / sizeof(int); i++) {
		std::ostringstream sin;
		for (int j = 0; j < strlens[i]; j++) {
			sin << "a";
		}

		sys::io::PopenStreambuf buf("/bin/echo", "-n", sin.str().c_str());
		StreambufReader reader(&buf);
		reader.start();
		reader.wait();

		REQUIRE(sin.str() == reader.str());
	}
}

} // namespace test_sys_io

#endif // TEST_SYS_IO_H
