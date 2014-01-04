/*
 * pepper - SCM statistics report generator
 * Copyright (C) 2010-2014 Jonas Gehring
 *
 * Released under the GNU General Public License, version 3.
 * Please see the COPYING file in the source distribution for license
 * terms and conditions, or see http://www.gnu.org/licenses/.
 *
 * file: syslib/datetime.cpp
 * Date and time functionality
 */


#include "main.h"

#include <cstring>

#include <sys/time.h>

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
	struct tm time;
	memset(&time, 0x00, sizeof(struct tm));
	if (strptime(str.c_str(), format.c_str(), &time) == NULL) {
		throw PEX(std::string("Error parsing time '" + str + "' with format '" + format + "'"));
	}
	return mktime(&time);
}

} // namespace datetime

} // namespace sys
