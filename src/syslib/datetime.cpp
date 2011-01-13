/*
 * pepper - SCM statistics report generator
 * Copyright (C) 2010 Jonas Gehring
 *
 * file: syslib/datetime.cpp
 * Date and time functionality
 */


#include <cassert>

#include <sys/time.h>

#include "main.h"

#include "datetime.h"


namespace sys 
{

namespace datetime
{

class WatchData
{
public:
	timeval tv;
};

// Constructor
Watch::Watch()
	: d(new WatchData)
{
	start();
}

// Destructor
Watch::~Watch()
{
	delete d;
}
		
// Starts the stop watch
void Watch::start()
{
	gettimeofday(&d->tv, NULL);
}

float Watch::elapsed() const
{
	timeval c;
	gettimeofday(&c, NULL);	
	return float(0.001f * ((c.tv_sec - d->tv.tv_sec) * 1000 + (c.tv_usec - d->tv.tv_usec) / 1000));
}

int Watch::elapsedMSecs() const
{
	timeval c;
	gettimeofday(&c, NULL);	
	return (c.tv_sec - d->tv.tv_sec) * 1000 + (c.tv_usec - d->tv.tv_usec) / 1000;
}


// Wrapper for strptime()
int64_t ptime(const std::string &str, const std::string &format)
{
	struct tm tm;
	if (strptime(str.c_str(), format.c_str(), &tm) == NULL) {
		return -1;
	}
	return mktime(&tm);
}


} // namespace datetime

} // namespace sys
