/*
 * pepper - SCM statistics report generator
 * Copyright (C) 2010 Jonas Gehring
 *
 * file: report.cpp
 * Lua interface for gathering repository data
 */


#include <cstdlib>
#include <iostream>

#include "backend.h"
#include "diffstat.h"
#include "globals.h"
#include "repository.h"
#include "revision.h"

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

// Pretty-prints a command-line option
static int print_option(lua_State *L)
{
	if (lua_gettop(L) != 2) {
		return luaL_error(L, "Invalid number of arguments (2 expected)");
	}
	utils::printOption(LuaHelpers::tops(L, -2), LuaHelpers::tops(L, -1));
	lua_pop(L, 2);
	return 0;
}

// Returns a script option (or the default value)
static int option(lua_State *L)
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
	Backend::RevisionIterator *it;
	try {
		it = backend->iterator(branch);
		backend->prepare(it);
	} catch (const Pepper::Exception &ex) {
		if (verbose) {
			std::cerr << "failed" << std::endl;
		}
		return LuaHelpers::pushError(L, ex.what(), ex.where());
	}

	if (verbose) {
		std::cerr << "done" << std::endl;
		std::cerr << "Mapping revisions... " << std::flush;
	}

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
	{"print_option", print_option},
	{"option", option},
	{"map_branch", map_branch},
	{NULL, NULL}
};


// Runs a scripted report using the given backend
int run(const std::string &script, Backend *backend)
{
	// Setup lua context
	lua_State *L = lua_open();
	luaL_openlibs(L);

	// Register report functions
	luaL_register(L, "pepper.report", report);

	// Register binding classes
	Lunar<LuaRepository>::Register(L, "pepper");
	Lunar<LuaRevision>::Register(L, "pepper");
	Lunar<LuaDiffstat>::Register(L, "pepper");
	Lunar<Plot>::Register(L, "pepper");

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

// Opens a script and calls the "print_help()" function
void printHelp(const std::string &script)
{
	// Setup lua context
	lua_State *L = lua_open();
	luaL_openlibs(L);

	// Register report functions
	luaL_register(L, "pepper.report", report);

	// Register binding classes
	Lunar<LuaRepository>::Register(L, "pepper");
	Lunar<LuaRevision>::Register(L, "pepper");
	Lunar<LuaDiffstat>::Register(L, "pepper");
	Lunar<Plot>::Register(L, "pepper");

	// Open the script
	if (luaL_dofile(L, script.c_str()) != 0) {
		throw PEX(utils::strprintf("Error opening report: %s", lua_tostring(L, -1)));
	}

	// Call the help function
	lua_getglobal(L, "print_help");
	if (lua_pcall(L, 0, 1, 0) != 0) {
		throw PEX(utils::strprintf("Error running report: %s", lua_tostring(L, -1)));
	}

	lua_gc(L, LUA_GCCOLLECT, 0);
	lua_close(L);
}

} // namespace Report
