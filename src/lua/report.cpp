/*
 * pepper - SCM statistics report generator
 * Copyright (C) 2010 Jonas Gehring
 *
 * file: report.cpp
 * Lua interface for gathering repository data
 */


#include <cstdlib>
#include <cstring>
#include <iostream>
#include <memory>

#include "backend.h"
#include "diffstat.h"
#include "globals.h"
#include "repository.h"
#include "revision.h"
#include "revisioniterator.h"

#include "luadiffstat.h"
#include "luahelpers.h"
#include "luarepository.h"
#include "luarevision.h"
#include "plot.h"

#include "options.h"
#include "utils.h"

#include "report.h"


namespace Report
{

// Global variables
static Repository *repo;
static LuaRepository *luarepo;
static std::map<std::string, std::string> options;


// Returns the current repository
static int repository(lua_State *L)
{
	return LuaHelpers::push(L, luarepo);
}

// Returns a script option (or the default value)
static int getopt(lua_State *L)
{
	if (lua_gettop(L) != 1 && lua_gettop(L) != 2) {
		return luaL_error(L, "Invalid number of arguments (1 or 2 expected)");
	}

	int narg = lua_gettop(L);

	std::vector<std::string> keys = utils::split(LuaHelpers::tops(L, (narg == 1 ? -1 : -2)), ",", true);
	std::string value = (narg == 2 ? LuaHelpers::tops(L, -1) : std::string());

	for (unsigned int i = 0; i < keys.size(); i++) {
		if (options.find(keys[i]) != options.end()) {
			value = options[keys[i]];
			break;
		}
	}
	lua_pop(L, narg);
	return LuaHelpers::push(L, value);
}

// Maps a lua function on all revisions of a given branch
static int map_branch(lua_State *L)
{
	if (lua_gettop(L) != 2) {
		return luaL_error(L, "Invalid number of arguments (2 expected)");
	}

	luaL_checktype(L, -2, LUA_TFUNCTION);
	std::string branch = LuaHelpers::pops(L);
	int callback = luaL_ref(L, LUA_REGISTRYINDEX);
	lua_pop(L, 1);

	bool verbose = (repo->backend()->options().verbosity() >= 0);
	if (verbose) {
		std::cerr << "Initializing iterator... " << std::flush;
	}

	Backend *backend = repo->backend();
	RevisionIterator *it;
	try {
		it = new RevisionIterator(branch, backend);
	} catch (const Pepper::Exception &ex) {
		if (verbose) {
			std::cerr << "failed" << std::endl;
		}
		return LuaHelpers::pushError(L, ex.what(), ex.where());
	}

	bool first = true;
	while (!it->atEnd()) {
		Revision *revision = NULL;
		try {
			revision = backend->revision(it->next());
		} catch (const Pepper::Exception &ex) {
			return LuaHelpers::pushError(L, ex.what(), ex.where());
		}
		LuaRevision luarev(revision);

		lua_rawgeti(L, LUA_REGISTRYINDEX, callback);
		LuaHelpers::push(L, &luarev);
		lua_call(L, 1, 1);
		lua_pop(L, 1);

		if (verbose) {
			if (first) {
				std::cerr << "done" << std::endl;
				first = false;
			}
			std::cerr << "\r\033[0K";
			std::cerr << "Mapping revisions... " << revision->id() << std::flush;
		}

		if (Globals::terminate) {
			return LuaHelpers::pushError(L, "Terminated");
		}

		delete revision;
	}

	if (verbose) {
		std::cerr << "\r\033[0K";
		std::cerr << "Mapping revisions... done" << std::endl;
	}

	try {
		backend->finalize();
	} catch (const Pepper::Exception &ex) {
		return LuaHelpers::pushError(L, ex.what(), ex.where());
	}
	delete it;
	return 0;
}

// Function table of the report library
static const struct luaL_reg report[] = {
	{"repository", repository},
	{"getopt", getopt},
	{"map_branch", map_branch},
	{NULL, NULL}
};


// Custom fclose() handler for lua file handles
static int utils_fclose(lua_State *L)
{
	FILE **p = (FILE **)lua_touserdata(L, 1);
	int rc = fclose(*p);
	if (rc == 0) *p = NULL;
	return 1;
}

// Generates a temporary file, and returns a file handle as well as the file name
static int utils_mkstemp(lua_State *L)
{
	std::string templ;
	if (lua_gettop(L) > 0) {
		templ = LuaHelpers::pops(L);
	} else {
		templ = "/tmp/pepperXXXXXX";
	}

	char *buf = strdup(templ.c_str());
	int fd = mkstemp(buf);

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

	*pf = fdopen(fd, "r+w");
	LuaHelpers::push(L, (const char *)buf);
	free(buf);
	return 2;
}

// Removes a file
static int utils_unlink(lua_State *L)
{
	if (lua_gettop(L) != 1) {
		return luaL_error(L, "Invalid number of arguments (1 expected)");
	}
	unlink(LuaHelpers::tops(L).c_str());
	return 0;
}

// Function table of the utils library
static const struct luaL_reg utils[] = {
	// TODO: Error handling for all functions
	{"mkstemp", utils_mkstemp},
	{"unlink", utils_unlink},
	{NULL, NULL}
};


// Sets up the lua context
lua_State *setupLua()
{
	// Setup lua context
	lua_State *L = lua_open();
	luaL_openlibs(L);

	// Register report functions
	luaL_register(L, "pepper.report", report);
	luaL_register(L, "pepper.utils", utils);

	// Register binding classes
	Lunar<LuaRepository>::Register(L, "pepper");
	Lunar<LuaRevision>::Register(L, "pepper");
	Lunar<LuaDiffstat>::Register(L, "pepper");
	Lunar<Plot>::Register(L, "pepper");

	// Setup meta table
	luaL_newmetatable(L, "meta");
	lua_setglobal(L, "meta");

	return L;
}

// Runs a scripted report using the given backend
int run(const std::string &script, Backend *backend)
{
	lua_State *L = setupLua();

	// Setup global variables
	Report::repo = new Repository(backend);
	Report::luarepo = new LuaRepository(repo);
	Report::options = backend->options().scriptOptions();

	// Run the script
	int ret = EXIT_SUCCESS;
	if (luaL_dofile(L, script.c_str()) != 0) {
		std::cerr << "Error opening report: " << lua_tostring(L, -1) << std::endl;
		ret = EXIT_FAILURE;
	} else {
		// Call the report function
		lua_getglobal(L, "main");
		if (lua_pcall(L, 0, 1, 0) != 0) {
			std::cerr << "Error running report: " << lua_tostring(L, -1) << std::endl;
			ret = EXIT_FAILURE;
		}
	}

	if (Globals::terminate) {
		ret = EXIT_FAILURE;
	}

	// Clean up
	lua_gc(L, LUA_GCCOLLECT, 0);
	lua_close(L);
	delete Report::repo;
	delete Report::luarepo;
	return ret;
}

// Opens a script, calls the "options()" function and pretty-prints some help
void printHelp(const std::string &script)
{
	lua_State *L = setupLua();

	// Open the script
	if (luaL_dofile(L, script.c_str()) != 0) {
		throw PEX(utils::strprintf("Error opening report: %s", lua_tostring(L, -1)));
	}

	lua_getglobal(L, "meta");

	// Retrieve the report name
	std::string name = script;
	lua_getfield(L, -1, "name");
	if (lua_type(L, -1) == LUA_TSTRING) {
		name = LuaHelpers::pops(L);
	}

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
		lua_pop(L, 1);

		std::cout << "Options for report '" << name << "':" << std::endl;
		for (unsigned int i = 0; i < std::min(switches.size(), text.size()); i++) {
			utils::printOption(switches[i], text[i]);
		}
	} else {
		std::cout << "No options for report '" << name << "':" << std::endl;
	}
	lua_pop(L, 1);

	lua_gc(L, LUA_GCCOLLECT, 0);
	lua_close(L);
}

} // namespace Report
