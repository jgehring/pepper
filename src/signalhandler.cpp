/*
 * pepper - SCM statistics report generator
 * Copyright (C) 2010 Jonas Gehring
 *
 * file: sighandler.cpp
 * Signal handler thread ensuring cache coherence
 */


#include <cstdlib>

#include <signal.h>

#include "main.h"

#include "cache.h"
#include "globals.h"

#include "signalhandler.h"


// Constructor
SignalHandler::SignalHandler()
	: m_cache(NULL)
{
	sigset_t set;
	sigemptyset(&set);
	sigaddset(&set, SIGINT);
	sigaddset(&set, SIGHUP);
	sigaddset(&set, SIGTERM);
	pthread_sigmask(SIG_BLOCK, &set, NULL);
}

// Sets the cache object
void SignalHandler::setCache(Cache *cache)
{
	m_cache = cache;
}

// Main thread function
void SignalHandler::run()
{
	int sig;
	sigset_t set;
	sigemptyset(&set);
	sigaddset(&set, SIGINT);
	sigaddset(&set, SIGHUP);
	sigaddset(&set, SIGTERM);

	if (sigwait(&set, &sig) == 0) {
		sys::parallel::MutexLocker locker(&Globals::cacheMutex);
		if (m_cache) {
			m_cache->flush();
		}
		exit(1);
	}
}
