/*
 * pepper - SCM statistics report generator
 * Copyright (C) 2010 Jonas Gehring
 *
 * file: diffstat.h
 * Diffstat object (interface)
 */


#ifndef DIFFSTAT_H_
#define DIFFSTAT_H_


#include <map>
#include <string>

#include "main.h"

class BIStream;
class BOStream;


class Diffstat
{
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
		Diffstat(std::istream &in);
		~Diffstat();

		void write(BOStream &out) const;
		bool load(BIStream &in);

	private:
		void parse(std::istream &in);

	private:
		std::map<std::string, Stat> m_stats;
};


#endif // DIFFSTAT_H_
