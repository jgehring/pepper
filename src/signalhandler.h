/*
 * pepper - SCM statistics report generator
 * Copyright (C) 2010 Jonas Gehring
 *
 * file: sighandler.h
 * Signal handler thread ensuring cache coherence (interface)
 */


#ifndef SIGNALHANDLER_H_
#define SIGNALHANDLER_H_


#include "syslib/parallel.h"

class Cache;


class SignalHandler : public sys::parallel::Thread
{
public:
	SignalHandler();

	void setCache(Cache *cache);

protected:
	void run();

private:
	Cache *m_cache;
};


#endif // SIGNALHANDLER_H_
