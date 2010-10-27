/*
 * pepper - SCM statistics report generator
 * Copyright (C) 2010 Jonas Gehring
 *
 * file: plot.cpp
 * Lua plotting interface using gnuplot
 */


#include <limits>

#include "gnuplot-cpp/gnuplot_i.h"

#include "utils.h"

#include "luahelpers.h"

#include "plot.h"


// Static variables for the lua bindings
const char Plot::className[] = "plot";
Lunar<Plot>::RegType Plot::methods[] = {
	LUNAR_DECLARE_METHOD(Plot, cmd),
	LUNAR_DECLARE_METHOD(Plot, set_output),
	LUNAR_DECLARE_METHOD(Plot, set_title),
	LUNAR_DECLARE_METHOD(Plot, plotxy),
	LUNAR_DECLARE_METHOD(Plot, plotty),
	{0,0}
};


// Constructor
Plot::Plot(lua_State *L)
{
	g = new Gnuplot("lines");
}

// Destructor
Plot::~Plot()
{
	delete g;
}

// Writes a Gnuplot command
int Plot::cmd(lua_State *L)
{
	g->cmd(LuaHelpers::check(L));
	return 0;
}

// Sets the output file name and optionally the terminal type
int Plot::set_output(lua_State *L)
{
	std::string file, terminal;
	if (lua_gettop(L) == 2) {
		terminal = LuaHelpers::check(L, -1);
		file = LuaHelpers::check(L, -2);
	} else {
		file = LuaHelpers::check(L, -1);
		// Determine terminal type from extension
		terminal = file.substr(file.find_last_of(".")+1);
		if (terminal.empty() || terminal == "ps") {
			terminal = "postscript";
		}
	}
	g->savetofigure(file, terminal);
	return 0;
}

// Sets the plot title
int Plot::set_title(lua_State *L)
{
	g->set_title(LuaHelpers::check(L));
	return 0;
}

// Plots normal data
int Plot::plotxy(lua_State *L)
{
	std::vector<double> keys, values;
	values = LuaHelpers::pop(L);
	keys = LuaHelpers::pop(L);

	try {
		g->plot_xy(keys, values);
	} catch (GnuplotException ex) {
		return LuaHelpers::pushError(L, ex.what());
	}
	return 0;
}

// Plots a time series
int Plot::plotty(lua_State *L)
{
	std::vector<double> keys, values;
	values = LuaHelpers::pop(L);
	keys = LuaHelpers::pop(L);

	// Run plot
	try {
		g->cmd("set xdata time");
		g->cmd("set timefmt \"%s\"");
		g->cmd("set format x \"%b %y\"");
		g->cmd("set format y \"%.0f\"");
		g->cmd("set yrange [0:*]");
		g->cmd("set xtics rotate by -45");
		g->cmd("set grid ytics");
		g->plot_xy(keys, values);
		g->replot();
	} catch (GnuplotException ex) {
		return LuaHelpers::pushError(L, ex.what());
    }
	return 0;
}
