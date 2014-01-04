/*
 * pepper - SCM statistics report generator
 * Copyright (C) 2010-2014 Jonas Gehring
 *
 * Released under the GNU General Public License, version 3.
 * Please see the COPYING file in the source distribution for license
 * terms and conditions, or see http://www.gnu.org/licenses/.
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

#include "lunar/lunar.h"

#include "syslib/parallel.h"

class BIStream;
class BOStream;


class Diffstat
{
	friend class DiffParser;

	public:
		struct Stat
		{
			uint64_t cadd, ladd;
			uint64_t cdel, ldel;

			Stat() : cadd(0), ladd(0), cdel(0), ldel(0) { }

			inline bool empty() const {
				return (cadd == 0 && ladd == 0 && cdel == 0 && ldel == 0);
			}
		};

	public:
		Diffstat();
		~Diffstat();

		std::map<std::string, Stat> stats() const;

		void filter(const std::string &prefix);

		void write(BOStream &out) const;
		bool load(BIStream &in);

	PEPPER_PVARS:
		std::map<std::string, Stat> m_stats;

	// Lua binding
	public:
		Diffstat(lua_State *L);

		int files(lua_State *L);
		int lines_added(lua_State *L);
		int bytes_added(lua_State *L);
		int lines_removed(lua_State *L);
		int bytes_removed(lua_State *L);

		static const char className[];
		static Lunar<Diffstat>::RegType methods[];
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
