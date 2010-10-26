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
#include "luahelpers.h"
#include "repository.h"
#include "revision.h"


namespace Report
{

// Maps a lua function on all revisions of a given branch
static int map_branch(lua_State *L)
{
	if (lua_gettop(L) != 3) {
		return luaL_error(L, "Invalid number of arguments (3 expected)");
	}

	luaL_checktype(L, -3, LUA_TFUNCTION);
	Repository *repository = Lunar<Repository>::check(L, -2);
	std::string branch = luaL_checkstring(L, -1);
	lua_pop(L, 2);
	int callback = luaL_ref(L, LUA_REGISTRYINDEX);
	lua_pop(L, 1);

	bool verbose = true;
	if (verbose) {
		std::cerr << "Initializing iterator... " << std::flush;
	}

	Backend *backend = repository->backend();
	Backend::RevisionIterator *it;
	try {
		it = backend->iterator(branch);
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
		Lunar<LuaRevision>::push(L, &luarev);
		lua_call(L, 1, 1);
		lua_pop(L, 1);

		if (verbose) {
			std::cerr << "\r\e[0K";
			std::cerr << "Mapping revisions... " << revision->id() << std::flush;
		}

		if (Globals::terminate) {
			return LuaHelpers::pushError(L, "Terminated");
		}

		delete revision;
	}

	if (verbose) {
		std::cerr << "\r\e[0K";
		std::cerr << "Mapping revisions... done" << std::endl;
	}

	delete it;
	return 0;
}

// Function table of the report library
static const struct luaL_reg report[] = {
	{"map_branch", map_branch},
	{NULL, NULL}
};


// Runs a scripted report using the given backend
int run(const char *script, Backend *backend)
{
	// Setup lua context
	lua_State *L = lua_open();
	luaL_openlibs(L);

	// Register report functions
	luaL_register(L, "pepper.report", report);

	// Register binding classes
	Lunar<Repository>::Register(L, "pepper");
	Lunar<LuaRevision>::Register(L, "pepper");
	Lunar<Diffstat>::Register(L, "pepper");

	// Push current repository backend to the stack
	Repository repo(backend);
	Lunar<Repository>::push(L, &repo);
	lua_setglobal(L, "g_repository");

	// Run the script
	int ret = EXIT_SUCCESS;
	if (luaL_dofile(L, script) != 0) {
		std::cerr << "Error running report: " << lua_tostring(L, -1) << std::endl;
		ret = EXIT_FAILURE;
	}

	if (Globals::terminate) {
		ret = EXIT_FAILURE;
	}

	// Clean up
	lua_gc(L, LUA_GCCOLLECT, 0);
	lua_close(L);
	return ret;
}

} // namespace Report
