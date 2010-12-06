/*
 * pepper - SCM statistics report generator
 * Copyright (C) 2010 Jonas Gehring
 *
 * file: logger.h
 * Logging facility class (interface)
 */


#ifndef LOGGER_H_
#define LOGGER_H_


#include <iostream>

#include "main.h"

// Easy logging
#ifndef POS_WIN
 #define PDEBUG (Logger::debug() << __PRETTY_FUNCTION__ << " [" << __LINE__ << "]: ")
 #define PTRACE (Logger::trace() << __PRETTY_FUNCTION__ << " [" << __LINE__  << "]: ")
#else
 #define PDEBUG (Logger::debug() << __FUNCTION__ << " [" << __LINE__ << "]: ")
 #define PTRACE (Logger::trace() << __FUNCTION__ << " [" << __LINE__  << "]: ")
#endif


enum LogModifier
{
	endl,
	flush
};

class Logger
{
	public:
		enum Level
		{
			None = 0,
			Status, Info, Debug, Trace,
			NumLevels
		};

	public:
		static void setOutput(std::ostream &out, int level = -1);
		static void setLevel(int level);
		static int level();

		static void flush();

		static inline Logger &status() { return *s_instances[Status]; }
		static inline Logger &info() { return *s_instances[Info]; }
		static inline Logger &debug() { return *s_instances[Debug]; }
		static inline Logger &trace() { return *s_instances[Trace]; }

		template <typename T>
		inline Logger &operator<<(const T &s) {
			if (m_level <= s_level) *m_out << s;
			return *this;
		}

		inline Logger &operator<<(std::ios_base& (* pf)(std::ios_base &)) {
			if (m_level <= s_level) *m_out << pf;
			return *this;
		}

		inline Logger &operator<<(LogModifier mod) {
			if (m_level > s_level) return *this;
			switch (mod) {
				case endl: *m_out << std::endl; break;
				case ::flush: *m_out << std::flush; break;
				default: break;
			}
			return *this;
		}

	public:
		Logger(int level, std::ostream &out);

	private:
		int m_level;
		std::ostream *m_out;

		static Logger *s_instances[NumLevels];
		static int s_level;
};


#endif // LOGGER_H_
