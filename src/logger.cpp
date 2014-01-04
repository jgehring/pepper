/*
 * pepper - SCM statistics report generator
 * Copyright (C) 2010-2014 Jonas Gehring
 *
 * Released under the GNU General Public License, version 3.
 * Please see the COPYING file in the source distribution for license
 * terms and conditions, or see http://www.gnu.org/licenses/.
 *
 * file: logger.cpp
 * Logging facility class (implementation)
 */


#include "main.h"

#include "logger.h"


// Static variables
Logger Logger::s_instances[Logger::NumLevels] = {
	Logger(Logger::None, std::cerr),
	Logger(Logger::Error, std::cerr),
	Logger(Logger::Warn, std::cerr),
	Logger(Logger::Status, std::cerr),
	Logger(Logger::Info, std::cerr),
	Logger(Logger::Debug, std::cerr),
	Logger(Logger::Trace, std::cerr)
};
#ifdef DEBUG
int Logger::s_level = Logger::Debug;
#else
int Logger::s_level = Logger::None;
#endif
sys::parallel::Mutex Logger::s_mutex;


// Constructor
Logger::Logger(int level, std::ostream &out)
	: m_level(level), m_out(&out)
{

}

// Sets the output device for the logger instances
void Logger::setOutput(std::ostream &out, int level)
{
	if (level < 0) {
		for (int i = 0; i < Logger::NumLevels; i++) {
			if (i != Logger::Error && i != Logger::Warn) {
				s_instances[i].m_out = &out;
			}
		}
	} else if (level != Logger::Error && level != Logger::Warn) {
		s_instances[level].m_out = &out;
	}
}

// Sets the log level
void Logger::setLevel(int level)
{
	s_level = std::max((int)Logger::Warn, level);
}

// Returns the log level
int Logger::level()
{
	return s_level;
}

// Flushes all loggers
void Logger::flush()
{
	for (int i = 0; i < Logger::NumLevels; i++) {
		s_instances[i].m_out->flush();
	}
}
