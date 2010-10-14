/*
 * pepper - SCM statistics report generator
 * Copyright (C) 2010 Jonas Gehring
 *
 * file: report.cpp
 * Lua interface for gathering repository data
 */


#include "backend.h"
#include "repository.h"
#include "revision.h"

#include "luahelpers.h"


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

	Backend *backend = repository->backend();
	Backend::RevisionIterator *it;
	try {
		it = backend->iterator(branch);
	} catch (const std::string &err) {
		return LuaHelpers::pushError(L, err);
	}

	while (!it->atEnd()) {
		Revision *revision = backend->revision(it->next());

		lua_rawgeti(L, LUA_REGISTRYINDEX, callback);
		Lunar<Revision>::push(L, revision);
		lua_call(L, 1, 1);
		lua_pop(L, 1);

		delete revision;
	}

	delete it;
	return 0;
}

// Function table of the report library
static const struct luaL_reg report[] = {
	{"map_branch", map_branch},
	{NULL, NULL}
};


// Registers the report library
int openLib(lua_State *L)
{
	luaL_openlib(L, "report", report, 0);
	return 1;
}

} // namespace Report
