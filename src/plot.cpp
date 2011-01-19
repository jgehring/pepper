/*
 * pepper - SCM statistics report generator
 * Copyright (C) 2010 Jonas Gehring
 *
 * file: plot.cpp
 * Lua plotting interface using gnuplot
 */


#include <limits>

#include "gnuplot-cpp/gnuplot_i.h"

#include "logger.h"
#include "utils.h"

#include "luahelpers.h"

#include "plot.h"


// Static variables for the lua bindings
const char Plot::className[] = "gnuplot";
Lunar<Plot>::RegType Plot::methods[] = {
	LUNAR_DECLARE_METHOD(Plot, cmd),
	LUNAR_DECLARE_METHOD(Plot, set_output),
	LUNAR_DECLARE_METHOD(Plot, set_title),
	LUNAR_DECLARE_METHOD(Plot, plot_series),
	LUNAR_DECLARE_METHOD(Plot, flush),
	{0,0}
};


// Constructor
Plot::Plot(lua_State *L)
{
	Gnuplot::set_terminal_std("svg");

	try {
		g = new Gnuplot();
	} catch (const GnuplotException &ex) {
		LuaHelpers::pushError(L, ex.what());
	}
}

// Destructor
Plot::~Plot()
{
	delete g;
}

// Writes a Gnuplot command
int Plot::cmd(lua_State *L)
{
	g->cmd(LuaHelpers::pops(L));
	return 0;
}

// Sets the output file name and optionally the terminal type
int Plot::set_output(lua_State *L)
{
	std::string file, terminal;
	int width = 640, height = 480;

	if (lua_gettop(L) > 4) {
		return LuaHelpers::pushError(L, utils::strprintf("Invalid number of arguments (expected 1-4, got %d)", lua_gettop(L)));
	}

	switch (lua_gettop(L)) {
		case 4: terminal = LuaHelpers::pops(L);
		case 3: height = LuaHelpers::popi(L);
		case 2: width = LuaHelpers::popi(L);
		default: file = LuaHelpers::pops(L);
	}

	if (terminal.empty()) {
		// Determine terminal type from extension or fall back to SVG
		size_t pos = file.find_last_of(".");
		if (pos != std::string::npos) {
			terminal = file.substr(pos+1);
			if (terminal.empty()) {
				terminal = "svg";
			} else if (terminal == "ps" || terminal == "eps") {
				terminal = "postscript";
			} else if (terminal == "jpg") {
				terminal = "jpeg";
			}
		} else {
			terminal = "svg";
		}
	}

	g->cmd(utils::strprintf("set terminal %s size %d,%d", terminal.c_str(), width, height));
	if (!file.empty()) {
		g->cmd(utils::strprintf("set output \"%s\"", file.c_str()));
	} else {
		g->cmd(utils::strprintf("set output"));
	}
	return 0;
}

// Sets the plot title
int Plot::set_title(lua_State *L)
{
	g->set_title(LuaHelpers::pops(L));
	return 0;
}

// Plots normal XY series
int Plot::plot_series(lua_State *L)
{
	// Validate arguments
	int index = -1;
	if (lua_gettop(L) > 4) {
		return LuaHelpers::pushError(L, utils::strprintf("Invalid number of arguments (expected 2-3, got %d)", lua_gettop(L)));
	}
	std::string style = "lines";
	switch (lua_gettop(L)) {
		case 4: style = LuaHelpers::pops(L);
		case 3: luaL_checktype(L, index--, LUA_TTABLE);
		default:
			luaL_checktype(L, index--, LUA_TTABLE);
			luaL_checktype(L, index--, LUA_TTABLE);
			break;
	}

	// First, read the keys (at index)
	++index;
	std::vector<double> keys = LuaHelpers::topvd(L, index);

	size_t nseries = 0;

	// Open stream to data file
	std::ofstream out;
	std::string file = g->create_tmpfile(out);

	// Read data entries and write them to a file
	++index;
	if (LuaHelpers::tablesize(L, index) != keys.size()) {
		return LuaHelpers::pushError(L, utils::strprintf("Number of keys and values doesn't match (%d != %d)", LuaHelpers::tablesize(L, index), keys.size()));
	}
	lua_pushvalue(L, index);
	lua_pushnil(L);
	int j = 0;
	while (lua_next(L, -2) != 0) {
		out << keys[j++] << " ";
		if (lua_type(L, -1) == LUA_TTABLE) {
			if (nseries == 0) {
				nseries = LuaHelpers::tablesize(L, -1);
			} else if (nseries != LuaHelpers::tablesize(L, -1)) {
				return LuaHelpers::pushError(L, "Inconsistent number of series");
			}
			
			lua_pushvalue(L, -1);
			lua_pushnil(L);
			while (lua_next(L, -2) != 0) {
				out << LuaHelpers::popd(L) << " ";
			}
			lua_pop(L, 2);
		} else {
			nseries = 1;
			out << LuaHelpers::popd(L);
		}
		out << std::endl;
	}
	lua_pop(L, 1);

	out.flush();
	out.close();

	// Read titles (if any)
	++index;
	std::vector<std::string> titles;
	if (index < 0) {
		titles = LuaHelpers::topvs(L, index);
	}

	std::ostringstream cmd;
	cmd << "plot ";
	for (size_t i = 0; i < nseries; i++) {
		cmd << "\"" << file << "\" using 1:" << (i+2);
		if (titles.size() > i) {
			cmd << " title \"" << titles[i] << "\"";
		} else {
			cmd << " notitle";
		}
		cmd << " with " << style;
		if (i < nseries-1) {
			cmd << ", ";
		}
	}
	PDEBUG << "Running plot with command: " << cmd.str() << endl;
	g->cmd(cmd.str());
	return 0;
}

// Closes and reopens the Gnuplot connection. This will force
// plotting to finish and temporary files to be closed
int Plot::flush(lua_State *L)
{
	try {
		delete g;
		g = new Gnuplot();
	} catch (const GnuplotException &ex) {
		return LuaHelpers::pushError(L, ex.what());
	}
	return 0;
}
