/*
 * pepper - SCM statistics report generator
 * Copyright (C) 2010-2011 Jonas Gehring
 *
 * Released under the GNU General Public License, version 3.
 * Please see the COPYING file in the source distribution for license
 * terms and conditions, or see http://www.gnu.org/licenses/.
 *
 * file: luamodules.h
 * Extra C modules for the Lua API (implementation)
 */


#include "main.h"

#include <cstring>

#include "cache.h"
#include "luahelpers.h"
#include "report.h"
#include "repository.h"

#include "syslib/datetime.h"
#include "syslib/fs.h"


namespace LuaModules
{

// "Main" module
namespace pepper
{

// Returns the current report context
int current_report(lua_State *L)
{
	if (Report::current() == NULL) {
		return LuaHelpers::pushNil(L);
	}
	return LuaHelpers::push(L, Report::current());
}

// Runs another report
int run(lua_State *L)
{
	Report r(L);
	return r.run(L);
}

// Returns a list of all reachable reports
int list_reports(lua_State *L)
{
	// Return only the report paths
	std::vector<std::pair<std::string, std::string> > reports = Report::listReports();
	std::vector<std::string> paths(reports.size());
	for (size_t i = 0; i < reports.size(); i++) {
		paths[i] = reports[i].first;
	}
	return LuaHelpers::push(L, paths);
}

// Function table of main functions
const struct luaL_reg table[] = {
	{"current_report", current_report},
	{"run", run},
	{"list_reports", list_reports},
	{NULL, NULL}
};

} // namespace pepper


// Utility functions
namespace utils
{

// Custom fclose() handler for lua file handles
int fclose(lua_State *L)
{
	FILE **p = (FILE **)lua_touserdata(L, 1);
	int rc = fclose(*p);
	if (rc == 0) *p = NULL;
	return 1;
}

// Generates a temporary file, and returns a file handle as well as the file name
int mkstemp(lua_State *L)
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
		lua_pushcfunction(L, fclose);
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
int unlink(lua_State *L)
{
	try {
		sys::fs::unlink(LuaHelpers::tops(L).c_str());
	} catch (const std::exception &ex) {
		return LuaHelpers::pushError(L, ex.what());
	}
	return 0;
}

// Splits a string
int split(lua_State *L)
{
	std::string pattern = LuaHelpers::pops(L);
	std::string string = LuaHelpers::pops(L);
	return LuaHelpers::push(L, ::utils::split(string, pattern));
}

// Wrapper for strptime
int strptime(lua_State *L)
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
int dirname(lua_State *L)
{
	return LuaHelpers::push(L, sys::fs::dirname(LuaHelpers::pops(L)));
}

// Wrapper for basename()
int basename(lua_State *L)
{
	return LuaHelpers::push(L, sys::fs::basename(LuaHelpers::pops(L)));
}

// Function table of the utils library
const struct luaL_reg table[] = {
	{"mkstemp", mkstemp},
	{"unlink", unlink},
	{"split", split},
	{"strptime", strptime},
	{"dirname", dirname},
	{"basename", basename},
	{NULL, NULL}
};

} // namespace utils


// Internal, undocumented functions
namespace internal
{

// Runs a cache check for the given repository
int check_cache(lua_State *L)
{
	Repository *repo = LuaHelpers::popl<Repository>(L);
	Cache *cache = dynamic_cast<Cache *>(repo->backend());
	if (cache == NULL) {
		cache = new Cache(repo->backend(), repo->backend()->options());
	}

	try {
		cache->check();
	} catch (const PepperException &ex) {
		return LuaHelpers::pushError(L, ::utils::strprintf("Error checking cache: %s: %s", ex.where(), ex.what()));
	}

	if (cache != repo->backend()) {
		delete cache;
	}
	return LuaHelpers::pushNil(L);
}

// Function table of internal functions
const struct luaL_reg table[] = {
	{"check_cache", check_cache},
	{NULL, NULL}
};

} // namespace internal


// Registers all modules in the given Lua context
void registerModules(lua_State *L)
{
	luaL_register(L, "pepper", pepper::table);
	luaL_register(L, "pepper.utils", utils::table);
	luaL_register(L, "pepper.internal", internal::table);
}

} // namespace LuaModules
