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

#include "lunar.h"


class Diffstat
{
	public:
		struct Stat
		{
			long cadd, ladd;
			long cdel, ldel;

			Stat() : cadd(0), ladd(0), cdel(0), ldel(0) { }
		};

	public:
		Diffstat();
		Diffstat(std::istream &in);
		Diffstat(lua_State *L);
		~Diffstat();

		// Lua bindings
		int files(lua_State *L);
		int stats(lua_State *L);
		int linesAdded(lua_State *L);
		int bytesAdded(lua_State *L);
		int linesRemoved(lua_State *L);
		int bytesRemoved(lua_State *L);

	public:
		static const char className[];
		static Lunar<Diffstat>::RegType methods[];

	private:
		void parse(std::istream &in);

	private:
		std::map<std::string, Stat> m_stats;
};


#endif // DIFFSTAT_H_
