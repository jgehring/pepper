/*
 * pepper - SCM statistics report generator
 * Copyright (C) 2010 Jonas Gehring
 *
 * file: luahelpersq.h
 * Helper functions for interacting with the Lua API
 */


#ifndef LUAHELPERS_H_
#define LUAHELPERS_H_


#include <string>
#include <vector>

#include "lunar.h"


namespace LuaHelpers
{

inline int push(lua_State *L, const std::string &s) {
	lua_pushstring(L, s.c_str());
	return 1;
}

inline int push(lua_State *L, const std::vector<std::string> &v) {
	lua_createtable(L, v.size(), 0);
	int table = lua_gettop(L);
	for (unsigned int i = 0; i < v.size(); i++) {
		lua_pushstring(L, v[i].c_str());
		lua_rawseti(L, table, i+1);
	}
	return 1;
}

inline int pushError(lua_State *L, const std::string &e) {
	lua_pushnil(L);
	lua_pushstring(L, e.c_str());
	return 2;
}

} // namespace LuaHelpers


#endif // LUAHELPERS_H_
