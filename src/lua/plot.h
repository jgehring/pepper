/*
 * pepper - SCM statistics report generator
 * Copyright (C) 2010 Jonas Gehring
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
		int plotxy(lua_State *L);
		int plotty(lua_State *L);

	public:
		static const char className[];
		static Lunar<Plot>::RegType methods[];

	private:
		Gnuplot *g;
};


#endif // PLOT_H_
