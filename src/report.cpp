/*
 * pepper - SCM statistics report generator
 * Copyright (C) 2010-2011 Jonas Gehring
 *
 * Released under the GNU General Public License, version 3.
 * Please see the COPYING file in the source distribution for license
 * terms and conditions, or see http://www.gnu.org/licenses/.
 *
 * file: report.cpp
 * Report script access and invocation, as well as a Lua interface proiding
 * main report actions
 */


#include "main.h"

#include <algorithm>
#include <cstring>
#include <sstream>
#include <stack>

#include "backend.h"
#include "cache.h"
#include "diffstat.h"
#include "logger.h"
#include "luahelpers.h"
#include "options.h"
#include "repository.h"
#include "revision.h"
#include "revisioniterator.h"
#include "tag.h"
#include "utils.h"
#ifdef USE_GNUPLOT
 #include "plot.h"
#endif

#include "syslib/datetime.h"
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


// Returns the current repository
int repository(lua_State *L)
{
	return LuaHelpers::push(L, Report::current()->repository());
}

// Returns a report option (or the default value)
int getopt(lua_State *L)
{
	if (lua_gettop(L) != 1 && lua_gettop(L) != 2) {
		return luaL_error(L, "Invalid number of arguments (1 or 2 expected)");
	}

	bool defaultProvided = (lua_gettop(L) == 2);
	std::string defaultValue = (lua_gettop(L) == 2 ? LuaHelpers::pops(L) : std::string());
	std::vector<std::string> keys = utils::split(LuaHelpers::pops(L), ",", true);
	const std::map<std::string, std::string> options = Report::current()->options();

	std::string value;
	bool valueFound = false;
	for (unsigned int i = 0; i < keys.size(); i++) {
		if (options.find(keys[i]) != options.end()) {
			value = options.find(keys[i])->second;
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

// Runs another report
int run_fromscript(lua_State *L)
{
	if (lua_gettop(L) != 1 && lua_gettop(L) != 2) {
		return luaL_error(L, "Invalid number of arguments (1 or 2 expected)");
	}

	std::map<std::string, std::string> options = Report::current()->options();
	if (lua_gettop(L) == 2) {
		options = LuaHelpers::popms(L);
	}
	std::string script = LuaHelpers::pops(L);

	Report r(findScript(script), options, Report::current()->repository()->backend());
	try {
		std::stringstream oss;
		if (r.run(oss) != 0) {
			return LuaHelpers::pushError(L, utils::trim(oss.str()));
		}
	} catch (const PepperException &ex) {
		return LuaHelpers::pushError(L, ex.what(), ex.where());
	} catch (const std::exception &ex) {
		return LuaHelpers::pushError(L, ex.what());
	}
	return LuaHelpers::pushNil(L);
}

// Backwards compability with 0.1
int revision(lua_State *L)
{
	return Report::current()->repository()->revision(L);
}

// Backwards compability with 0.1
int walk_branch(lua_State *L)
{
	if (lua_gettop(L) < 2 || lua_gettop(L) > 4) {
		return luaL_error(L, "Invalid number of arguments (2 to 4 expected)");
	}

	std::string branch;
	int64_t start = -1, end = -1;
	switch (lua_gettop(L)) {
		case 4: end = LuaHelpers::popi(L);
		case 3: start = LuaHelpers::popi(L);
		case 2: branch = LuaHelpers::pops(L);
		default: break;
	}

	luaL_checktype(L, -1, LUA_TFUNCTION);
	int callback = luaL_ref(L, LUA_REGISTRYINDEX);

	// First, get an iterator
	LuaHelpers::push(L, branch);
	LuaHelpers::push(L, start);
	LuaHelpers::push(L, end);
	Report::current()->repository()->iterator(L);
	RevisionIterator *it = LuaHelpers::popl<RevisionIterator>(L);

	// Map callback function
	lua_rawgeti(L, LUA_REGISTRYINDEX, callback);
	it->map(L);

	delete it;
	return 0;
}

// Function table of the report library
const struct luaL_reg report[] = {
	{"getopt", getopt},
	{"repository", repository},
	{"revision", revision},
	{"walk_branch", walk_branch},
	{"run", run_fromscript},
	{NULL, NULL}
};


// Custom fclose() handler for lua file handles
int utils_fclose(lua_State *L)
{
	FILE **p = (FILE **)lua_touserdata(L, 1);
	int rc = fclose(*p);
	if (rc == 0) *p = NULL;
	return 1;
}

// Generates a temporary file, and returns a file handle as well as the file name
int utils_mkstemp(lua_State *L)
{
	std::string templ;
	if (lua_gettop(L) > 0) {
		templ = LuaHelpers::pops(L);
	}

	FILE **pf = (FILE **)lua_newuserdata(L, sizeof *pf);
	*pf = 0;
	luaL_getmetatable(L, LUA_FILEHANDLE);
	lua_setmetatable(L, -2);

	// Register custom __close() function
	// (From lua posix module by Luiz Henrique de Figueiredo)
	lua_getfield(L, LUA_REGISTRYINDEX, "PEPPER_UTILS_FILE");
	if (lua_isnil(L, -1)) {
		lua_pop(L, 1);
		lua_newtable(L);
		lua_pushvalue(L, -1);
		lua_pushcfunction(L, utils_fclose);
		lua_setfield(L, -2, "__close");
		lua_setfield(L, LUA_REGISTRYINDEX, "PEPPER_UTILS_FILE");
	}
	lua_setfenv(L, -2);

	// Gemerate the file
	std::string filename;
	try {
		*pf = sys::fs::mkstemp(&filename, templ);
	} catch (const PepperException &ex) {
		return LuaHelpers::pushError(L, ex.what(), ex.where());
	}

	LuaHelpers::push(L, filename);
	return 2;
}

// Removes a file
int utils_unlink(lua_State *L)
{
	try {
		sys::fs::unlink(LuaHelpers::tops(L).c_str());
	} catch (const std::exception &ex) {
		return LuaHelpers::pushError(L, ex.what());
	}
	return 0;
}

// Splits a string
int utils_split(lua_State *L)
{
	std::string pattern = LuaHelpers::pops(L);
	std::string string = LuaHelpers::pops(L);
	return LuaHelpers::push(L, utils::split(string, pattern));
}

// Wrapper for strptime
int utils_strptime(lua_State *L)
{
	std::string format = LuaHelpers::pops(L);
	std::string str = LuaHelpers::pops(L);
	int64_t time;
	try {
		time = sys::datetime::ptime(str, format);
	} catch (const std::exception &ex) {
		return LuaHelpers::pushError(L, ex.what());
	}
	return LuaHelpers::push(L, time);
}

// Wrapper for dirname()
int utils_dirname(lua_State *L)
{
	return LuaHelpers::push(L, sys::fs::dirname(LuaHelpers::pops(L)));
}

// Wrapper for basename()
int utils_basename(lua_State *L)
{
	return LuaHelpers::push(L, sys::fs::basename(LuaHelpers::pops(L)));
}

// Function table of the utils library
const struct luaL_reg utils[] = {
	{"mkstemp", utils_mkstemp},
	{"unlink", utils_unlink},
	{"split", utils_split},
	{"strptime", utils_strptime},
	{"dirname", utils_dirname},
	{"basename", utils_basename},
	{NULL, NULL}
};


// Runs a cache check for the given repository
int internal_check_cache(lua_State *L)
{
	Repository *repo = LuaHelpers::popl<Repository>(L);
	Cache *cache = dynamic_cast<Cache *>(repo->backend());
	if (cache == NULL) {
		cache = new Cache(repo->backend(), repo->backend()->options());
	}

	try {
		cache->check();
	} catch (const PepperException &ex) {
		return LuaHelpers::pushError(L, utils::strprintf("Error checking cache: %s: %s", ex.where(), ex.what()));
	}

	if (cache != repo->backend()) {
		delete cache;
	}
	return LuaHelpers::pushNil(L);
}

// Function table of internal functions
const struct luaL_reg internal[] = {
	{"check_cache", internal_check_cache},
	{NULL, NULL}
};

// Sets up the lua context
lua_State *setupLua()
{
	// Setup lua context
	lua_State *L = lua_open();
	luaL_openlibs(L);

	lua_atpanic(L, atpanic);

	// Register report functions
	luaL_register(L, "pepper.report", report);
	luaL_register(L, "pepper.utils", utils);
	luaL_register(L, "pepper.internal", internal);

	// Register binding classes
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
		// Call the report function
		lua_getglobal(L, "main");
		if (lua_pcall(L, 0, 1, 0) != 0) {
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
