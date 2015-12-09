/*
 * pepper - SCM statistics report generator
 * Copyright (C) 2010-present Jonas Gehring
 *
 * Released under the GNU General Public License, version 3.
 * Please see the COPYING file in the source distribution for license
 * terms and conditions, or see http://www.gnu.org/licenses/.
 *
 * file: tests/units/test_parallel.h
 * Unit tests for POSIX thread classes 
 */


#ifndef TEST_PARALLEL_H
#define TEST_PARALLEL_H


#include <sys/time.h> 

#include "syslib/parallel.h"


namespace test_parallel
{

// Example thread for testing
class SleepThread : public sys::parallel::Thread
{
public:
	SleepThread(int msecs = 100) : sys::parallel::Thread() {
		m_msecs = msecs;
	}

	void run() {
		msleep(m_msecs);
	}

	int m_msecs;
};


TEST_CASE("parallel/msleep", "parallel::Thread::msleep()")
{
	struct timeval t1, t2;
	double elapsed;

	SECTION("5", "5ms") {
		gettimeofday(&t1, 0);
		sys::parallel::Thread::msleep(5);
		gettimeofday(&t2, 0);
		elapsed = (t2.tv_sec - t1.tv_sec) * 1000.0 + (t2.tv_usec - t1.tv_usec) / 1000.0;
		REQUIRE(fabs(5-elapsed) < 1.5);
	}

	SECTION("10", "10ms") {
		gettimeofday(&t1, 0);
		sys::parallel::Thread::msleep(10);
		gettimeofday(&t2, 0);
		elapsed = (t2.tv_sec - t1.tv_sec) * 1000.0 + (t2.tv_usec - t1.tv_usec) / 1000.0;
		REQUIRE(fabs(10-elapsed) < 1.5);
	}

	SECTION("50", "50ms") {
		gettimeofday(&t1, 0);
		sys::parallel::Thread::msleep(50);
		gettimeofday(&t2, 0);
		elapsed = (t2.tv_sec - t1.tv_sec) * 1000.0 + (t2.tv_usec - t1.tv_usec) / 1000.0;
		REQUIRE(fabs(50-elapsed) < 1.5);
	}
}

TEST_CASE("parallel/thread", "parallel::Thread")
{
	SleepThread thread(10);

	SECTION("wait", "Normal wait") {
		for (int i = 0; i < 3; i++) {
			thread.start();
			thread.wait();
		}
	}

	SECTION("finished_wait", "Wait after finished") {
		thread.start();
		sys::parallel::Thread::msleep(thread.m_msecs + 10);
		thread.wait();
	}

	SECTION("abort", "Abort") {
		thread.start();
		thread.abort();

		thread.start();
		thread.wait();
	}
}

} // namespace test_parallel


#endif // TEST_PARALLEL_H
