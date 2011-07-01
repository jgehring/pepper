/*
 * pepper - SCM statistics report generator
 * Copyright (C) 2010-2011 Jonas Gehring
 *
 * Released under the GNU General Public License, version 3.
 * Please see the COPYING file in the source distribution for license
 * terms and conditions, or see http://www.gnu.org/licenses/.
 *
 * file: report.cpp
 * Report script context
 */


#include "main.h"

#include <algorithm>
#include <cstring>
#include <sstream>
#include <stack>

#include "backend.h"
#include "diffstat.h"
#include "logger.h"
#include "luahelpers.h"
#include "luamodules.h"
#include "options.h"
#include "repository.h"
#include "revision.h"
#include "revisioniterator.h"
#include "tag.h"
#include "utils.h"
#ifdef USE_GNUPLOT
 #include "plot.h"
#endif

#include "syslib/fs.h"

#include "report.h"

#define PEPPER_MAX_STACK_SIZE 64


/*
 * Stack helper functions
 */

namespace {

// Checks if the given report is "executable", i.e. contains a main() function
bool isExecutable(lua_State *L)
{
	lua_getglobal(L, "main");
	bool hasmain = (lua_type(L, -1) == LUA_TFUNCTION);
	lua_pop(L, 1);
	return hasmain;
}

// Prints a backtrace if Lua is panicking
int atpanic(lua_State *L)
{
	std::cerr << "Lua PANIC: " << LuaHelpers::tops(L) << std::endl;
#ifdef DEBUG
	std::cerr << PepperException::stackTrace();
#endif
	return 0;
}

// Returns all paths that may contains reports
std::vector<std::string> reportDirs()
{
	std::vector<std::string> dirs;

	// Read the PEPPER_REPORTS environment variable
	char *env = getenv("PEPPER_REPORTS");
	if (env != NULL) {
#ifdef POS_WIN
		std::vector<std::string> parts = utils::split(env, ";");
#else
		std::vector<std::string> parts = utils::split(env, ":");
#endif
		for (size_t i = 0; i < parts.size(); i++) {
			dirs.push_back(parts[i]);
		}
	}

#ifdef DATADIR
	dirs.push_back(DATADIR);
#endif
	return dirs;
}

// Returns the full path to the given script
std::string findScript(const std::string &script)
{
	if (sys::fs::exists(script)) {
		return script;
	}
	if (sys::fs::exists(script + ".lua")) {
		return script + ".lua";
	}

	std::vector<std::string> dirs = reportDirs();
	for (size_t i = 0; i < dirs.size(); i++) {
		std::string path = dirs[i] + "/" + script;
		if (sys::fs::exists(path)) {
			return path;
		}
		path += ".lua";
		if (sys::fs::exists(path)) {
			return path;
		}
	}
	return script;
}

// Sets up the lua context
lua_State *setupLua()
{
	// Setup lua context
	lua_State *L = lua_open();
	luaL_openlibs(L);

	lua_atpanic(L, atpanic);

	// Register extra modules functions
	LuaModules::registerModules(L);

	// Register binding classes
	Lunar<Report>::Register(L, "pepper");
	Lunar<Repository>::Register(L, "pepper");
	Lunar<Revision>::Register(L, "pepper");
	Lunar<RevisionIterator>::Register(L, "pepper");
	Lunar<Diffstat>::Register(L, "pepper");
	Lunar<Tag>::Register(L, "pepper");
#ifdef USE_GNUPLOT
	Lunar<Plot>::Register(L, "pepper");
#endif

	// Setup package path to include built-in modules
	lua_getglobal(L, "package");
	lua_getfield(L, -1, "path");
	std::string path = LuaHelpers::pops(L);
	std::vector<std::string> dirs = reportDirs();
	for (size_t i = 0; i < dirs.size(); i++) {
		path += ";";
		path += dirs[i] + "/?.lua";
	}
	LuaHelpers::push(L, path);
	lua_setfield(L, -2, "path");
	lua_pop(L, 1);
	PDEBUG << "Lua package path has been set to " << path << endl;

	// Setup meta table
	luaL_newmetatable(L, "meta");
	lua_setglobal(L, "meta");

	return L;
}

} // anonymous namespace


/*
 * Report class implementation
 */

// Static report stack
std::stack<Report *> Report::s_stack;

// Constructor
Report::Report(const std::string &script, Backend *backend)
	: m_repo(NULL), m_script(script)
{
	if (backend) {
		m_repo = new Repository(backend);
		m_options = backend->options().reportOptions();
	}
}

// Constructor
Report::Report(const std::string &script, const std::map<std::string, std::string> &options, Backend *backend)
	: m_repo(NULL), m_script(script), m_options(options)
{
	if (backend) {
		m_repo = new Repository(backend);
	}	
}

// Destructor
Report::~Report()
{
	delete m_repo;
}


// Runs the report, printing error messages to the given stream
int Report::run(std::ostream &err)
{
	if (s_stack.size() == PEPPER_MAX_STACK_SIZE) {
		err << "Error running report: maximum stack size (";
		err << PEPPER_MAX_STACK_SIZE << ") exceeded" << std::endl;
		return EXIT_FAILURE;
	}

	// Push this context on top of the stack
	std::string path = findScript(m_script);
	PDEBUG << "Report path resolved: " << m_script << " -> " << path << endl;
	PTRACE << "Pushing report context for " << path << endl;
	s_stack.push(this);
	PDEBUG << "Stack size is " << s_stack.size() << endl;

	lua_State *L = setupLua();

	// Run the script
	int ret = EXIT_SUCCESS;
	if (luaL_dofile(L, path.c_str()) != 0) {
		err << "Error opening report: " << lua_tostring(L, -1) << std::endl;
		ret = EXIT_FAILURE;
	} else if (!isExecutable(L)) {
		err << "Error opening report '" << path << "': Not executable" << std::endl;
		ret = EXIT_FAILURE;
	} else {
		// Call the report function, with the current report context as an argument
		lua_getglobal(L, "main");
		LuaHelpers::push(L, this);
		if (lua_pcall(L, 1, 1, 0) != 0) {
			err << "Error running report: " << lua_tostring(L, -1) << std::endl;
			ret = EXIT_FAILURE;
		}
	}

	// Clean up
	lua_gc(L, LUA_GCCOLLECT, 0);
	lua_close(L);

	PTRACE << "Popping report context for " << path <<  endl;
	s_stack.pop();
	return ret;
}

// Pretty-prints report options
void Report::printHelp()
{
	lua_State *L = setupLua();

	// Open the script
	std::string path = findScript(m_script);
	if (luaL_dofile(L, path.c_str()) != 0) {
		throw PEX(utils::strprintf("Error opening report: %s", lua_tostring(L, -1)));
	}

	lua_getglobal(L, "meta");

	// Retrieve the report name
	std::string name = m_script;
	lua_getfield(L, -1, "title");
	if (lua_type(L, -1) == LUA_TSTRING) {
		name = LuaHelpers::tops(L);
	} else {
		lua_pop(L, 1);
		// "meta.name" has been used in the first version of the scripting tutorial
		lua_getfield(L, -1, "name");
		if (lua_type(L, -1) == LUA_TSTRING) {
			name = LuaHelpers::tops(L);
		}
	}
	lua_pop(L, 1);

	std::cout << "Options for report '" << name << "':" << std::endl;

	// Check for possible options
	lua_getfield(L, -1, "options");
	if (lua_type(L, -1) == LUA_TTABLE) {
		// Pop the arguments from the stack
		std::vector<std::string> switches, text;
		luaL_checktype(L, -1, LUA_TTABLE);
		lua_pushnil(L);
		while (lua_next(L, -2) != 0) {
			luaL_checktype(L, -1, LUA_TTABLE);
			lua_pushnil(L);
			int i = 0;
			while (lua_next(L, -2) != 0) {
				if (i == 0) {
					switches.push_back(luaL_checkstring(L, -1));
				} else if (i == 1) {
					text.push_back(luaL_checkstring(L, -1));
				}
				++i;
				lua_pop(L, 1);
			}
			lua_pop(L, 1);
		}

		for (unsigned int i = 0; i < std::min(switches.size(), text.size()); i++) {
			Options::print(switches[i], text[i]);
		}
	}
	lua_pop(L, 1);

	lua_gc(L, LUA_GCCOLLECT, 0);
	lua_close(L);
}

// Prints a listing of all report scripts to stdout
void Report::listReports(std::ostream &out)
{
	std::vector<std::string> dirs = reportDirs();
	for (size_t j = 0; j < dirs.size(); j++) {
		std::string builtin = dirs[j];
		if (!sys::fs::dirExists(builtin)) {
			continue;
		}

		out << "Available reports in " << builtin << ":" << std::endl;

		std::vector<std::string> reports = sys::fs::ls(builtin);
		std::sort(reports.begin(), reports.end());

		for (size_t i = 0; i < reports.size(); i++) {
			if (reports[i].empty() || reports[i][reports[i].length()-1] == '~' || !sys::fs::fileExists(builtin + "/" + reports[i])) {
				continue;
			}

			lua_State *L = setupLua();
			std::string path = builtin + "/" + reports[i];
			if (luaL_dofile(L, path.c_str()) != 0) {
				PDEBUG << "Error opening report at " << path << ": " << lua_tostring(L, -1) << endl;
				lua_gc(L, LUA_GCCOLLECT, 0);
				lua_close(L);
				continue;
			}

			if (!isExecutable(L)) {
				PDEBUG << "Skipping unexecutable report " << path << endl;
				lua_gc(L, LUA_GCCOLLECT, 0);
				lua_close(L);
				continue;
			}

			lua_getglobal(L, "meta");

			// Retrieve the report description
			std::string description;
			lua_getfield(L, -1, "description");
			if (lua_type(L, -1) == LUA_TSTRING) {
				description = LuaHelpers::tops(L);
			}
			lua_pop(L, 1);

			// Strip possible Lua suffix when displaying
			std::string name = reports[i];
			if (name.length() > 4 && name.compare(name.length()-4, 4, ".lua") == 0) {
				name = name.substr(0, name.length()-4);
			}

			Options::print(name, description, out);
			lua_gc(L, LUA_GCCOLLECT, 0);
			lua_close(L);
		}

		if (j < dirs.size()-1) {
			out << std::endl;
		}
	}

#ifndef USE_GNUPLOT
	out << std::endl << "NOTE: Built without Gnuplot support. ";
	out << "Graphical reports are not shown." << std::endl;
#endif
}

// Returns the backend wrapper
Repository *Report::repository() const
{
	return m_repo;
}

// Returns the report options
std::map<std::string, std::string> Report::options() const
{
	return m_options;
}

// Returns the current context
Report *Report::current()
{
	return (s_stack.empty() ? NULL : s_stack.top());
}

/*
 * Lua binding
 */

const char Report::className[] = "report";
Lunar<Report>::RegType Report::methods[] = {
	LUNAR_DECLARE_METHOD(Report, getopt),
	LUNAR_DECLARE_METHOD(Report, repository),
	LUNAR_DECLARE_METHOD(Report, run),
	{0,0}
};

Report::Report(lua_State *L)
	: m_repo(NULL)
{
	if (lua_gettop(L) != 1 && lua_gettop(L) != 2) {
		LuaHelpers::pushError(L, "Invalid number of arguments (1 or 2 expected)");
		return;
	}
	if (!Report::current()) {
		LuaHelpers::pushError(L, "No current report");
		return;
	}

	m_options = Report::current()->options();
	if (lua_gettop(L) == 2) {
		m_options = LuaHelpers::popms(L);
	}
	m_script = LuaHelpers::pops(L);

	m_repo = new Repository(Report::current()->repository()->backend());
}

int Report::repository(lua_State *L)
{
	return LuaHelpers::push(L, m_repo);
}

int Report::getopt(lua_State *L)
{
	if (lua_gettop(L) != 1 && lua_gettop(L) != 2) {
		return luaL_error(L, "Invalid number of arguments (1 or 2 expected)");
	}

	bool defaultProvided = (lua_gettop(L) == 2);
	std::string defaultValue = (lua_gettop(L) == 2 ? LuaHelpers::pops(L) : std::string());
	std::vector<std::string> keys = utils::split(LuaHelpers::pops(L), ",", true);

	std::string value;
	bool valueFound = false;
	for (unsigned int i = 0; i < keys.size(); i++) {
		if (m_options.find(keys[i]) != m_options.end()) {
			value = m_options.find(keys[i])->second;
			valueFound = true;
			break;
		}
	}

	if (valueFound) {
		return LuaHelpers::push(L, value);
	} else if (defaultProvided) {
		return LuaHelpers::push(L, defaultValue);
	}
	return LuaHelpers::pushNil(L);
}

int Report::run(lua_State *L)
{
	try {
		std::stringstream oss;
		if (run(oss) != 0) {
			return LuaHelpers::pushError(L, utils::trim(oss.str()));
		}
	} catch (const PepperException &ex) {
		return LuaHelpers::pushError(L, ex.what(), ex.where());
	} catch (const std::exception &ex) {
		return LuaHelpers::pushError(L, ex.what());
	}
	return LuaHelpers::pushNil(L);
}
