/*
 * pepper - SCM statistics report generator
 * Copyright (C) 2010-2011 Jonas Gehring
 *
 * Released under the GNU General Public License, version 3.
 * Please see the COPYING file in the source distribution for license
 * terms and conditions, or see http://www.gnu.org/licenses/.
 *
 * file: syslib/datetime.h
 * Date and time functionality (interfaces)
 */


#ifndef SYS_DATETIME_H_
#define SYS_DATETIME_H_


#include <string>


namespace sys
{

namespace datetime
{

class WatchData;
class Watch
{
	public:
		Watch();
		~Watch();
		
		void start();
		float elapsed() const;
		int elapsedMSecs() const;

	private:
		WatchData *d;
};


int64_t ptime(const std::string &str, const std::string &format);

} // namespace time

} // namespace sys


#endif // SYS_DATETIME_H_
