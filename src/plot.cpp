/*
 * pepper - SCM statistics report generator
 * Copyright (C) 2010-2012 Jonas Gehring
 *
 * Released under the GNU General Public License, version 3.
 * Please see the COPYING file in the source distribution for license
 * terms and conditions, or see http://www.gnu.org/licenses/.
 *
 * file: plot.cpp
 * Lua plotting interface using gnuplot
 */


#include "main.h"

#include <cmath>
#include <cstring>
#include <fstream>

#include "gnuplot.h"
#include "logger.h"
#include "luahelpers.h"
#include "options.h"
#include "report.h"
#include "strlib.h"

#include "syslib/io.h"
#include "syslib/fs.h"

#include "plot.h"


// Converts from UNIX to Gnuplot epoch
static inline int64_t convepoch(int64_t t)
{
	return t - 946684800;
}

// Gnuplot arguments (-persist only works with the X11 or wxt terminal)
static const char *gp_args[] = {NULL};
static const char *gp_args_persist[] = {"-persist", NULL};


// Static variables for the lua bindings
const char Plot::className[] = "gnuplot";
Lunar<Plot>::RegType Plot::methods[] = {
	LUNAR_DECLARE_METHOD(Plot, cmd),
	LUNAR_DECLARE_METHOD(Plot, set_output),
	LUNAR_DECLARE_METHOD(Plot, set_title),
	LUNAR_DECLARE_METHOD(Plot, set_xrange),
	LUNAR_DECLARE_METHOD(Plot, set_xrange_time),
	LUNAR_DECLARE_METHOD(Plot, plot_series),
	LUNAR_DECLARE_METHOD(Plot, plot_multi_series),
	LUNAR_DECLARE_METHOD(Plot, plot_histogram),
	LUNAR_DECLARE_METHOD(Plot, plot_pie),
	LUNAR_DECLARE_METHOD(Plot, flush),
	{0,0}
};


bool Plot::s_hasX11Term = false;
bool Plot::s_detectTerminals = true;


// Constructor
Plot::Plot(lua_State *L)
{
	m_standardTerminal = "svg";
	m_args = gp_args;

#if ( defined(unix) || defined(__unix) || defined(__unix__) ) && !defined(__APPLE__)
	if (getenv("DISPLAY") && sys::io::isterm(stdout) && !Report::current()->outputRedirected()) {
		m_standardTerminal = "x11";
		detectTerminals();
	}
#endif
	if (getenv("GNUTERM")) {
		m_standardTerminal = getenv("GNUTERM");
	}

	if (m_standardTerminal == "x11" && s_hasX11Term) {
		m_args = gp_args_persist;
	}

	try {
		g = new Gnuplot(m_args, Report::current()->out());
	} catch (const PepperException &ex) {
		LuaHelpers::pushError(L, ex.what(), ex.where());
	}
}

// Destructor
Plot::~Plot()
{
	delete g;
	removeTempfiles();
}

// Writes a Gnuplot command
int Plot::cmd(lua_State *L)
{
	gcmd(LuaHelpers::pops(L));
	return 0;
}

// Sets the output file name and optionally the terminal type
int Plot::set_output(lua_State *L)
{
	std::string file, terminal;
	int width = 640, height = 480;

	if (lua_gettop(L) > 4) {
		return LuaHelpers::pushError(L, str::printf("Invalid number of arguments (expected 1-4, got %d)", lua_gettop(L)));
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
				terminal = m_standardTerminal;
			}
		} else {
			terminal = m_standardTerminal;
		}
	}

	if (terminal == "ps" || terminal == "eps") {
		terminal = "postscript eps color enhanced";
	} else if (terminal == "jpg") {
		terminal = "jpeg";
	}

	if (!file.empty()) {
		gcmd(str::printf("set output \"%s\"", file.c_str()));
	} else {
		gcmd(str::printf("set output"));
	}
	gcmd(str::printf("set terminal %s size %d,%d", terminal.c_str(), width, height));
	return 0;
}

// Sets the plot title
int Plot::set_title(lua_State *L)
{
	std::string title = LuaHelpers::pops(L);
	gcmd(str::printf("set title \"%s\"", title.c_str()));
	return 0;
}

// Sets the xaxis and x2axis range
int Plot::set_xrange(lua_State *L)
{
	double end = LuaHelpers::popd(L);
	double start = LuaHelpers::popd(L);
	double d = end - start;

	double range[2];
	range[0] = 1000 * floor(double(start) - 0.05 * d) / 1000;
	range[1] = 1000 * ceil(double(end) + 0.05 * d) / 1000;

	gcmd(str::printf("set xrange [%f:%f]", range[0], range[1]));
	gcmd(str::printf("set x2range [%f:%f]", range[0], range[1]));
	return 0;
}

// Sets the xaxis and x2axis range
int Plot::set_xrange_time(lua_State *L)
{
	int64_t end = LuaHelpers::popi(L);
	int64_t start = LuaHelpers::popi(L);
	int64_t d = end - start;

	int64_t range[2];
	range[0] = convepoch(1000 * floor(double(start) - 0.05 * d) / 1000);
	range[1] = convepoch(1000 * ceil(double(end) + 0.05 * d) / 1000);

	gcmd(str::printf("set xrange [%lld:%lld]", range[0], range[1]));
	gcmd(str::printf("set x2range [%lld:%lld]", range[0], range[1]));
	return 0;
}

// Plots normal XY series
int Plot::plot_series(lua_State *L)
{
	// Validate arguments
	int index = -1;
	if (lua_gettop(L) > 4) {
		return LuaHelpers::pushError(L, str::printf("Invalid number of arguments (expected 2-4, got %d)", lua_gettop(L)));
	}

	std::map<std::string, std::string> options;
	options.insert(std::pair<std::string, std::string>("style", "lines"));
	switch (lua_gettop(L)) {
		case 4: {
			if (lua_type(L, -1) == LUA_TTABLE) {
				options = LuaHelpers::popms(L);
			} else {
				options["style"] = LuaHelpers::pops(L);
			}
		}
		case 3: luaL_checktype(L, index--, LUA_TTABLE);
		default:
			luaL_checktype(L, index--, LUA_TTABLE);
			luaL_checktype(L, index--, LUA_TTABLE);
			break;
	}

	// First, read the keys (at index)
	++index;
	std::vector<double> keys = LuaHelpers::topvd(L, index);

	// Check data entries
	size_t nseries = 0;
	++index;
	if (LuaHelpers::tablesize(L, index) != keys.size()) {
		return LuaHelpers::pushError(L, str::printf("Number of keys and values doesn't match (%d != %d)", LuaHelpers::tablesize(L, index), keys.size()));
	}

	lua_pushvalue(L, index);
	lua_pushnil(L);
	if (lua_next(L, -2) != 0) {
		if (lua_type(L, -1) == LUA_TTABLE) {
			nseries = LuaHelpers::tablesize(L, -1);
		} else {
			nseries = 1;
		}
		lua_pop(L, 2);
	}
	lua_pop(L, 1);

	// Read titles (if any)
	++index;
	std::vector<std::string> titles;
	if (index < 0) {
		titles = LuaHelpers::topvs(L, index);
	}

	std::ostringstream cmd;
	cmd << "plot ";
	if (options.find("command") == options.end()) {
		for (size_t i = 0; i < nseries; i++) {
			cmd << (i == 0 ? "'-'" : "''") << " using 1:2";
			if (titles.size() > i) {
				cmd << " title \"" << titles[i] << "\"";
			} else {
				cmd << " notitle";
			}
			if (options.find("style") != options.end()) {
				cmd << " with " << options["style"];
			}
			if (i < nseries-1) {
				cmd << ", ";
			}
		}
	} else {
		cmd << "'-' " << options["command"];
	}
	PDEBUG << "Running plot with command: " << cmd.str() << endl;
	gcmd(cmd.str());

	// Write data to pipe, separately for each series
	--index;
	std::ostringstream ss;
	for (size_t i = 0; i < nseries; i++) {
		ss.clear(); // Reset stringstream, but keep buffer
		ss.seekp(0);

		lua_pushvalue(L, index);
		lua_pushnil(L);
		int j = 0;
		while (lua_next(L, -2) != 0) {
			ss << keys[j++] << " ";
			if (lua_type(L, -1) == LUA_TTABLE) {
				if (nseries != LuaHelpers::tablesize(L, -1)) {
					return LuaHelpers::pushError(L, "Inconsistent number of series");
				}


				lua_pushnumber(L, i+1);
				lua_gettable(L, -2);
				ss << LuaHelpers::popd(L);
				lua_pop(L, 1);
			} else {
				ss << LuaHelpers::popd(L);
			}
			ss << "\n";
		}
		lua_pop(L, 1);

		ss << "e\n"; // Marks end of data
		g->cmd(ss.str().c_str(), ss.tellp());
	}
	return 0;
}

// Plots multiple XY series
int Plot::plot_multi_series(lua_State *L)
{
	// Validate arguments
	int index = -1;
	if (lua_gettop(L) > 4) {
		return LuaHelpers::pushError(L, str::printf("Invalid number of arguments (expected 2-4, got %d)", lua_gettop(L)));
	}

	std::map<std::string, std::string> options;
	options.insert(std::pair<std::string, std::string>("style", "lines"));
	switch (lua_gettop(L)) {
		case 4: {
			if (lua_type(L, -1) == LUA_TTABLE) {
				options = LuaHelpers::popms(L);
			} else {
				options["style"] = LuaHelpers::pops(L);
			}
		}
		case 3: luaL_checktype(L, index--, LUA_TTABLE);
		default:
			luaL_checktype(L, index--, LUA_TTABLE);
			luaL_checktype(L, index--, LUA_TTABLE);
			break;
	}

	// Read data
	++index;
	size_t nseries = LuaHelpers::tablesize(L, index);

	std::ostringstream *outs = new std::ostringstream[nseries];
	for (size_t i = 0; i < nseries; i++) {
		std::ostringstream &out = outs[i];

		// Read keys
		lua_rawgeti(L, index, i+1);
		std::vector<double> keys = LuaHelpers::popvd(L);

		// Check number of values
		++index;
		lua_rawgeti(L, index, i+1);
		if (LuaHelpers::tablesize(L) != keys.size()) {
			return LuaHelpers::pushError(L, str::printf("Number of keys and values doesn't match (%d != %d)", LuaHelpers::tablesize(L, index), keys.size()));
		}

		// Avoid copying values via popvd() and read the directly
		luaL_checktype(L, -1, LUA_TTABLE);
		lua_pushvalue(L, -1);
		lua_pushnil(L);
		size_t j = 0;
		while (lua_next(L, -2) != 0) {
			out << keys[j++] << " " << LuaHelpers::popd(L) << std::endl;
		}
		lua_pop(L, 2);

		// Reset index back to keys
		--index;
	}

	// Read titles (if any)
	index += 2;
	std::vector<std::string> titles;
	if (index < 0) {
		titles = LuaHelpers::topvs(L, index);
	}

	std::ostringstream cmd;
	cmd << "plot ";
	for (size_t i = 0; i < nseries; i++) {
		cmd << "'-' using 1:2";
		if (titles.size() > i) {
			cmd << " title \"" << titles[i] << "\"";
		} else {
			cmd << " notitle";
		}
		if (options.find("style") != options.end()) {
			cmd << " with " << options["style"];
		}
		if (i < nseries-1) {
			cmd << ", ";
		}
	}
	PDEBUG << "Running plot with command: " << cmd.str() << endl;
	gcmd(cmd.str());

	// Write data
	for (size_t i = 0; i < nseries; i++) {
		g->cmd(outs[i].str());
		g->cmd("e"); // Marks end of data
	}

	delete[] outs;
	return 0;
}

// Plots a histogram
int Plot::plot_histogram(lua_State *L)
{
	// Validate arguments
	int index = -1;
	if (lua_gettop(L) > 4) {
		return LuaHelpers::pushError(L, str::printf("Invalid number of arguments (expected 2-4, got %d)", lua_gettop(L)));
	}

	std::map<std::string, std::string> options;
	switch (lua_gettop(L)) {
		case 4: {
			if (lua_type(L, -1) == LUA_TTABLE) {
				options = LuaHelpers::popms(L);
			} else {
				options["style"] = LuaHelpers::pops(L);
			}
		}
		case 3: luaL_checktype(L, index--, LUA_TTABLE);
		default:
			luaL_checktype(L, index--, LUA_TTABLE);
			luaL_checktype(L, index--, LUA_TTABLE);
			break;
	}

	// First, read the keys (at index)
	++index;
	std::vector<std::string> keys = LuaHelpers::topvs(L, index);

	// Check data entries
	size_t nseries = 0;
	++index;
	if (LuaHelpers::tablesize(L, index) != keys.size()) {
		return LuaHelpers::pushError(L, str::printf("Number of keys and values doesn't match (%d != %d)", LuaHelpers::tablesize(L, index), keys.size()));
	}

	lua_pushvalue(L, index);
	lua_pushnil(L);
	if (lua_next(L, -2) != 0) {
		if (lua_type(L, -1) == LUA_TTABLE) {
			nseries = LuaHelpers::tablesize(L, -1);
		} else {
			nseries = 1;
		}
		lua_pop(L, 2);
	}
	lua_pop(L, 1);

	// Read titles (if any)
	++index;
	std::vector<std::string> titles;
	if (index < 0) {
		titles = LuaHelpers::topvs(L, index);
	}

	gcmd("set style data histogram");
	std::ostringstream cmd;
	cmd << "plot ";
	for (size_t i = 0; i < nseries; i++) {
		cmd << (i == 0 ? "'-'" : "''") << " using  2:xtic(1)";
		if (titles.size() > i) {
			cmd << " title \"" << titles[i] << "\"";
		} else {
			cmd << " notitle";
		}
		if (options.find("style") != options.end()) {
			cmd << " with " << options["style"];
		}
		if (i < nseries-1) {
			cmd << ", ";
		}
	}
	PDEBUG << "Running plot with command: " << cmd.str() << endl;
	gcmd(cmd.str());

	// Write data to pipe, separately for each series
	--index;
	std::ostringstream ss;
	for (size_t i = 0; i < nseries; i++) {
		ss.clear(); // Reset stringstream, but keep buffer
		ss.seekp(0);

		lua_pushvalue(L, index);
		lua_pushnil(L);
		int j = 0;
		while (lua_next(L, -2) != 0) {
			ss << '"' << keys[j++] << "\" ";
			if (lua_type(L, -1) == LUA_TTABLE) {
				if (nseries != LuaHelpers::tablesize(L, -1)) {
					return LuaHelpers::pushError(L, "Inconsistent number of series");
				}


				lua_pushnumber(L, i+1);
				lua_gettable(L, -2);
				ss << LuaHelpers::popd(L);
				lua_pop(L, 1);
			} else {
				ss << LuaHelpers::popd(L);
			}
			ss << "\n";
		}
		lua_pop(L, 1);

		ss << "e\n"; // Marks end of data
		g->cmd(ss.str().c_str(), ss.tellp());
	}
	return 0;
}

// Plots a pie chart
int Plot::plot_pie(lua_State *L)
{
	// Validate arguments
	if (lua_gettop(L) != 2) {
		return LuaHelpers::pushError(L, str::printf("Invalid number of arguments (expected 2, got %d)", lua_gettop(L)));
	}

	std::vector<double> values;
	std::vector<std::string> keys;
	switch (lua_gettop(L)) {
		default: 
			values = LuaHelpers::popvd(L);
			keys = LuaHelpers::popvs(L);
			break;
	}
	if (keys.size() != values.size()) {
		return LuaHelpers::pushError(L, str::printf("Argument dimensions don't match (%d != %d)", keys.size(), values.size()));
	}

	// Prepare data, i.e. accumulate values to get [from,to] intervals
	size_t n = keys.size();
	for (size_t i = 1; i < n; i++) {
		values[i] += values[i-1];
	}

	// Open stream to data file
	std::ostringstream cmd;

	// First, print plot-specific data and helper functions to the file
	cmd <<
"set parametric\n"
"set trange [0:1]\n"
"set xrange [-1:1]\n"
"set yrange [-1:1]\n"
"set offsets 0.25,0.25,0.25,0.25\n" // For percentage labels
"unset border\n"
"unset tics\n"
"p2rad(x)=pi * (x/0.5)\n"
"fs(t,s,e)=sin(t * p2rad((e-s)) + p2rad(s))\n"
"fc(t,s,e)=cos(t * p2rad((e-s)) + p2rad(s))" << std::endl;

	// Print percentage labels
	double last = 0;
	for (size_t i = 0; i < n; i++) {
		double m = last + (values[i] - last) / 2.0;
		cmd << "set label " << i+1 << " \"" << int(100 * (values[i] - last) + 0.5) << "%\" at first " <<
			"1.05*cos(p2rad(" << m << ")),1.05*sin(p2rad(" << m << ")) front " <<
			(m > 0.25 && m < 0.76 ? "right" : "left") << std::endl;
		last = values[i];
	}

	// Plot arcs
	cmd << "plot \\" << std::endl;
	last = 0;
	for (size_t i = 0; i < n; i++) {
		cmd << "  fc(t," << last << "," << values[i] << "),fs(t," << last << "," << values[i] <<
		   ") w filledcu xy=0,0 title " << "\"" << keys[i] << "\"" <<
		   (i != n-1 ? ",\\" : "") << std::endl;
		last = values[i];
	}

	gcmd(cmd.str());
	return 0;
}

// Closes and reopens the Gnuplot connection. This will force
// plotting to finish and temporary files to be closed
int Plot::flush(lua_State *L)
{
	try {
		delete g;
		g = new Gnuplot(m_args, Report::current()->out());
	} catch (const PepperException &ex) {
		return LuaHelpers::pushError(L, ex.what(), ex.where());
	}
	return 0;
}

// Sends a command to GNUPlot (and logs it)
void Plot::gcmd(const std::string &c)
{
	PTRACE << c << endl;
	g->cmd(c);
}

// Creates a temporary file
std::string Plot::tempfile(std::ofstream &out)
{
	std::string path;
	sys::fs::mkstemp(&path);

	out.open(path.c_str());
	if (out.bad()) {
		throw PEX(str::printf("Unable to open temporary file '%s'", path.c_str()));
	}

	m_tempfiles.push_back(path);
	return path;
}

// Removes all temporary files
void Plot::removeTempfiles()
{
	for (size_t i = 0; i < m_tempfiles.size(); i++) {
		sys::fs::unlink(m_tempfiles[i]);
	}
	m_tempfiles.clear();
}

// Detects Gnuplot terminals, and checks wheter the X11 terminal is available
void Plot::detectTerminals()
{
	if (!s_detectTerminals) { // Run only once
		return;
	}

	std::string terms;
	try {
		// Set dummy pager for terminal listing
		char *oldPager = getenv("PAGER");
		if (oldPager) {
			oldPager = strdup(oldPager);
		}
		setenv("PAGER", "cat", 1);

		int ret;
		std::string path = sys::fs::which("gnuplot");
		terms = sys::io::exec(&ret, path.c_str(), "-e", "set terminal; quit");
		if (ret != 0) {
			throw PEX(terms);
		}

		if (!oldPager) {
			unsetenv("PAGER");
		} else {
			setenv("PAGER", oldPager, 1);
			free(oldPager);
		}
	} catch (const std::exception &ex) {
		Logger::info() << "Can't query list of supported Gnuplot terminals ("
			<< ex.what() << ")" << std::endl;
	}

	s_hasX11Term = (terms.find("x11") != std::string::npos);
	s_detectTerminals = false;
}
