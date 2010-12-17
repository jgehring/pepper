/*
 * pepper - SCM statistics report generator
 * Copyright (C) 2010 Jonas Gehring
 *
 * file: logger.cpp
 * Logging facility class (implementation)
 */


#include "logger.h"


// Static variables
Logger *Logger::s_instances[Logger::NumLevels] = {
	new Logger(Logger::None, std::cout),
	new Logger(Logger::Error, std::cerr),
	new Logger(Logger::Status, std::cout),
	new Logger(Logger::Info, std::cout),
	new Logger(Logger::Debug, std::cout),
	new Logger(Logger::Trace, std::cout)
};
#ifdef DEBUG
int Logger::s_level = Logger::Debug;
#else
int Logger::s_level = Logger::None;
#endif


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
			if (i != Logger::Error) {
				s_instances[i]->m_out = &out;
			}
		}
	} else if (level != Logger::Error) {
		s_instances[level]->m_out = &out;
	}
}

// Sets the log level
void Logger::setLevel(int level)
{
	s_level = std::max((int)Logger::Error, level);
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
		s_instances[i]->m_out->flush();
	}
}
