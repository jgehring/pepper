/*
 * pepper - SCM statistics report generator
 * Copyright (C) 2010-2012 Jonas Gehring
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


#include <vector>
#include <iostream>

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
		int plot_pie(lua_State *L);

		int flush(lua_State *L);

	public:
		static const char className[];
		static Lunar<Plot>::RegType methods[];

	private:
		void gcmd(const std::string &c);
		std::string tempfile(std::ofstream &out);
		void removeTempfiles();
		static void detectTerminals();

	private:
		Gnuplot *g;
		std::vector<std::string> m_tempfiles;
		std::string m_standardTerminal;
		const char **m_args;
		static bool s_hasX11Term;
		static bool s_detectTerminals;
};


#endif // PLOT_H_
