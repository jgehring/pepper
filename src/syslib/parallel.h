/*
 * pepper - SCM statistics report generator
 * Copyright (C) 2010-2011 Jonas Gehring
 *
 * Released under the GNU General Public License, version 3.
 * Please see the COPYING file in the source distribution for license
 * terms and conditions, or see http://www.gnu.org/licenses/.
 *
 * file: syslib/parallel.h
 * POSIX thread classes and utilities (interface)
 */


#ifndef SYS_PARALLEL_H_
#define SYS_PARALLEL_H_


#include <string>

#include <pthread.h>
#include <semaphore.h>


namespace sys
{

namespace parallel
{

int idealThreadCount();

class Mutex
{
	friend class WaitCondition;

	public:
		Mutex();
		~Mutex();

		void lock();
		void unlock();

	private:
		pthread_mutex_t m_pmx;
};


class MutexLocker
{
	public:
		MutexLocker(Mutex *mutex);
		~MutexLocker();

		void relock();
		void unlock();

	private:
		Mutex *m_mutex;
};


class Thread
{
	public:
		Thread();
		virtual ~Thread();

		void start();
		void abort();
		void wait();
		bool running();

		static void msleep(int msecs);

	protected:
		virtual void run() = 0;

	private:
		static void *main(void *obj);
		static void cleanup(void *obj);
		void setupAndRun();

	private:
		pthread_t m_pth;
		volatile int m_running;
		Mutex m_mutex;
};


class WaitCondition
{
	public:
		WaitCondition();
		~WaitCondition();

		void wait(Mutex *mutex);
		void wake();
		void wakeAll();

	private:
		pthread_cond_t m_pcond;
};


class Semaphore
{
	public:
		Semaphore(int n = 0);
		~Semaphore();

		int available();
		void acquire(int n = 1);
		int maxAcquire(int n = 1);
		void release(int n = 1);

	private:
		int m_avail;
		pthread_mutex_t m_mutex;
		pthread_cond_t m_cond;
};

} // namespace parallel

} // namespace sys


#endif // SYS_PARALLEL_H_
