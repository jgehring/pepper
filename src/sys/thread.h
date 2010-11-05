/*
 * pepper - SCM statistics report generator
 * Copyright (C) 2010 Jonas Gehring
 *
 * file: sys/thread.h
 * POSIX thread classes and utilities (interface)
 */


#ifndef SYS_THREAD_H_
#define SYS_THREAD_H_


#include <string>

#include <pthread.h>
#include <semaphore.h>


namespace sys
{

namespace thread
{

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
		virtual ~Thread() { }

		void start();
		void abort();
		void wait();

		static void msleep(int msecs);

	protected:
		virtual void run() = 0;

		bool abortFlag();

	private:
		static void *main(void *obj);

	private:
		pthread_t m_pth;
		volatile bool m_running;
		Mutex m_mutex;
		bool m_abort;
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

} // namespace thread

} // namespace sys


#endif // SYS_THREAD_H_
