/*
 * pepper - SCM statistics report generator
 * Copyright (C) 2010 Jonas Gehring
 *
 * file: diffstat.h
 * Diffstat object and parser (interface)
 */


#ifndef DIFFSTAT_H_
#define DIFFSTAT_H_


#include <iostream>
#include <map>
#include <string>

#include "main.h"

#include "syslib/parallel.h"

class BIStream;
class BOStream;


class Diffstat
{
	friend class DiffParser;
	friend class LuaDiffstat;

	public:
		struct Stat
		{
			uint64_t cadd, ladd;
			uint64_t cdel, ldel;

			Stat() : cadd(0), ladd(0), cdel(0), ldel(0) { }
		};

	public:
		Diffstat();
		~Diffstat();

		void write(BOStream &out) const;
		bool load(BIStream &in);

	private:
		std::map<std::string, Stat> m_stats;
};


class DiffParser : public sys::parallel::Thread
{
	public:
		DiffParser(std::istream &in);

		Diffstat stat() const;

		static Diffstat parse(std::istream &in);

	protected:
		void run();

	private:
		std::istream &m_in;
		Diffstat m_stat;
};


#endif // DIFFSTAT_H_
