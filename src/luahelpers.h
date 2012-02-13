/*
 * pepper - SCM statistics report generator
 * Copyright (C) 2010-2012 Jonas Gehring
 *
 * Released under the GNU General Public License, version 3.
 * Please see the COPYING file in the source distribution for license
 * terms and conditions, or see http://www.gnu.org/licenses/.
 *
 * file: luahelpersq.h
 * Helper functions for interacting with the Lua API
 */


#ifndef LUAHELPERS_H_
#define LUAHELPERS_H_


#include "main.h"

#include <iostream>
#include <map>
#include <string>
#include <vector>

#include "strlib.h"

#include "lunar/lunar.h"


namespace LuaHelpers
{

inline void stackdump(lua_State *L, std::ostream &out = std::cout) {
	int top = lua_gettop(L);
	out << "Stack size: " << top << std::endl;
	for (int i = top; i >= 1; i--) {
		out << "  -" << (top-i+1) << " ";
		int t = lua_type(L, i);
		switch (t) {
			case LUA_TSTRING:
				out << "string: '" << lua_tostring(L, i) << "'";
				break;
			case LUA_TBOOLEAN:
				out << "boolean: " << (lua_toboolean(L, i) ? "true" : "false");
				break;
			case LUA_TNUMBER:
				out << "number: " << lua_tonumber(L, i);
				break;
			default:
				out << lua_typename(L, t);
				break;
		}
		out << std::endl;
	}
}

inline int push(lua_State *L, int i) {
	lua_pushinteger(L, i);
	return 1;
}

inline int push(lua_State *L, int64_t i) {
	lua_pushinteger(L, i);
	return 1;
}

inline int push(lua_State *L, uint64_t i) {
	lua_pushnumber(L, i);
	return 1;
}

inline int push(lua_State *L, double i) {
	lua_pushnumber(L, i);
	return 1;
}

inline int push(lua_State *L, bool i) {
	lua_pushboolean(L, (int)i);
	return 1;
}

inline int push(lua_State *L, const char *s) {
	lua_pushstring(L, s);
	return 1;
}

inline int push(lua_State *L, const std::string &s) {
	lua_pushlstring(L, s.c_str(), s.length());
	return 1;
}

inline int push(lua_State *L, int (*f)(lua_State *L)) {
	lua_pushcfunction(L, f);
	return 1;
}

template <typename T>
inline int push(lua_State *L, const std::vector<T> &v)
{
	lua_createtable(L, v.size(), 0);
	int table = lua_gettop(L);
	int i = 1;
	typename std::vector<T>::const_iterator it = v.begin();
	while (it != v.end()) {
		push(L, *it);
		lua_rawseti(L, table, i++);
		++it;
	}
	return 1;
}

template <typename T1, typename T2>
inline int push(lua_State *L, const std::map<T1, T2> &m) {
	lua_createtable(L, m.size(), 0);
	int table = lua_gettop(L);
	typename std::map<T1, T2>::const_iterator it = m.begin();
	while (it != m.end()) {
		push(L, it->first);
		push(L, it->second);
		lua_settable(L, table);
		++it;
	}
	return 1;
}

template <class T>
inline int push(lua_State *L, T *i, bool gc = false)
{
	Lunar<T>::push(L, i, gc);
	return 1;
}

inline int pushNil(lua_State *L) {
	lua_pushnil(L);
	return 0;
}

inline int pushError(lua_State *L, const std::string &e) {
	// Make sure that the string is passed properly
	return luaL_error(L, "%s", e.c_str());
}

inline int pushError(lua_State *L, const char *what, const char *where) {
	std::string s(where);
	s += ": ";
	s += what;
	return pushError(L, s);
}


inline int topi(lua_State *L, int index = -1) {
	return luaL_checkinteger(L, index);
}

inline bool topb(lua_State *L, int index = -1) {
	luaL_checktype(L, index, LUA_TBOOLEAN);
	return (bool)lua_toboolean(L, index);
}

inline double topd(lua_State *L, int index = -1) {
	return luaL_checknumber(L, index);
}

inline std::string tops(lua_State *L, int index = -1) {
	return luaL_checkstring(L, index);
}

template <class T>
inline T *topl(lua_State *L, int index = -1) {
	return Lunar<T>::check(L, index);
}

inline int popi(lua_State *L) {
	int i = topi(L, -1); lua_pop(L, 1); return i;
}

inline bool popb(lua_State *L) {
	bool b = topb(L, -1); lua_pop(L, 1); return b;
}

inline double popd(lua_State *L) {
	double d = topd(L, -1); lua_pop(L, 1); return d;
}

inline std::string pops(lua_State *L) {
	std::string s = tops(L, -1); lua_pop(L, 1); return s;
}

template <class T>
inline T* popl(lua_State *L) {
	T *t = topl<T>(L); lua_pop(L, 1); return t;
}

inline std::vector<double> topvd(lua_State *L, int index = -1) {
	std::vector<double> t;
	luaL_checktype(L, index, LUA_TTABLE);
	lua_pushvalue(L, index);
	lua_pushnil(L);
	while (lua_next(L, -2) != 0) {
		t.push_back(popd(L));
	}
	lua_pop(L, 1);
	return t;
}

inline std::vector<std::string> topvs(lua_State *L, int index = -1) {
	std::vector<std::string> t;
	luaL_checktype(L, index, LUA_TTABLE);
	lua_pushvalue(L, index);
	lua_pushnil(L);
	while (lua_next(L, -2) != 0) {
		t.push_back(pops(L));
	}
	lua_pop(L, 1);
	return t;
}

inline std::map<std::string, std::string> topms(lua_State *L, int index = -1) {
	std::map<std::string, std::string> t;
	luaL_checktype(L, index, LUA_TTABLE);
	lua_pushvalue(L, index);
	lua_pushnil(L);
	while (lua_next(L, -2) != 0) {
		t[tops(L, -2)] = tops(L, -1);
		lua_pop(L, 1);
	}
	lua_pop(L, 1);
	return t;
}

inline std::vector<double> popvd(lua_State *L) {
	std::vector<double> t = topvd(L, -1); lua_pop(L, 1); return t;
}

inline std::vector<std::string> popvs(lua_State *L) {
	std::vector<std::string> t = topvs(L, -1); lua_pop(L, 1); return t;
}

inline std::map<std::string, std::string> popms(lua_State *L) {
	std::map<std::string, std::string> t = topms(L, -1); lua_pop(L, 1); return t;
}


template<typename T1>
inline void call(lua_State *L, const T1 &arg1, int nresults) {
	push(L, arg1);
	lua_call(L, 1, nresults);
}

template<typename T1, typename T2>
inline void call(lua_State *L, const T1 &arg1, const T2 &arg2, int nresults) {
	push(L, arg1);
	push(L, arg2);
	lua_call(L, 2, nresults);
}

template<typename T1, typename T2, typename T3>
inline void call(lua_State *L, const T1 &arg1, const T2 &arg2, const T3 &arg3, int nresults) {
	push(L, arg1);
	push(L, arg2);
	push(L, arg3);
	lua_call(L, 3, nresults);
}

template<typename T1, typename T2>
inline void calls(lua_State *L, const std::string &name, const T1 &arg1, const T2 &arg2) {
	// Find function
	std::vector<std::string> parts = str::split(name, ".");

	lua_getglobal(L, parts[0].c_str());
	if (parts.size() > 1) {
		for (size_t i = 1; i < parts.size(); i++) {
			if (lua_type(L, -1) != LUA_TTABLE) {
				throw PEX("No such module");
			}
			lua_getfield(L, -1, parts[i].c_str());
			lua_remove(L, -2);
		}
	}

	if (lua_type(L, -1) != LUA_TFUNCTION) {
		throw PEX("No such function");
	}
	push(L, arg1);
	push(L, arg2);
	lua_call(L, 2, 1);
}


inline int tablevi(lua_State *L, const std::string &key, int def = -1, int index = -1) {
	luaL_checktype(L, index, LUA_TTABLE);
	push(L, key);
	lua_gettable(L, index-1);
	if (lua_isnil(L, -1)) {
		lua_pop(L, 1);
		return def;
	}
	return popi(L);
}

inline bool tablevb(lua_State *L, const std::string &key, bool def = false, int index = -1) {
	luaL_checktype(L, index, LUA_TTABLE);
	push(L, key);
	lua_gettable(L, index-1);
	if (lua_isnil(L, -1)) {
		lua_pop(L, 1);
		return def;
	}
	return popb(L);
}

inline std::string tablevb(lua_State *L, const std::string &key, const std::string &def = std::string(), int index = -1) {
	luaL_checktype(L, index, LUA_TTABLE);
	push(L, key);
	lua_gettable(L, index-1);
	if (lua_isnil(L, -1)) {
		lua_pop(L, 1);
		return def;
	}
	return pops(L);
}


inline size_t tablesize(lua_State *L, int index = -1) {
	return lua_objlen(L, index);
}

inline bool hasFunction(lua_State *L, const std::string &name) {
	lua_getglobal(L, name.c_str());
	bool has = (lua_type(L, -1) == LUA_TFUNCTION);
	lua_pop(L, 1);
	return has;
}

} // namespace LuaHelpers


#endif // LUAHELPERS_H_
