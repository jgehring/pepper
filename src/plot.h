/*
 * pepper - SCM statistics report generator
 * Copyright (C) 2010-2011 Jonas Gehring
 *
 * Released under the GNU General Public License, version 3.
 * Please see the COPYING file in the source distribution for license
 * terms and conditions, or see http://www.gnu.org/licenses/.
 *
 * file: plot.h
 * Lua plotting interface using gnuplot (interface)
 */


#ifndef PLOT_H_
#define PLOT_H_


#include "lunar/lunar.h"

class Gnuplot;


class Plot
{
	public:
		Plot(lua_State *L);
		~Plot();

		int cmd(lua_State *L);
		int set_output(lua_State *L);
		int set_title(lua_State *L);
		int set_xrange(lua_State *L);
		int set_xrange_time(lua_State *L);

		int plot_series(lua_State *L);
		int plot_multi_series(lua_State *L);
		int plot_histogram(lua_State *L);

		int flush(lua_State *L);

	public:
		static const char className[];
		static Lunar<Plot>::RegType methods[];

	private:
		void gcmd(const std::string &c);

	private:
		Gnuplot *g;
};


#endif // PLOT_H_
