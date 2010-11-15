/*
 * pepper - SCM statistics report generator
 * Copyright (C) 2010 Jonas Gehring
 *
 * file: luadiffstat.h
 * Lua bindings for the Diffstat class 
 */


#ifndef LUADIFFSTAT_H_
#define LUADIFFSTAT_H_


#include "diffstat.h"
#include "luahelpers.h"


class LuaDiffstat
{
	typedef Diffstat::Stat Stat;

	public:
		LuaDiffstat(Diffstat *stat) : w(stat) { }
		LuaDiffstat(lua_State *L) : w(NULL) { }

		int files(lua_State *L) {
			if (w == NULL) return LuaHelpers::pushNil(L);
			std::vector<std::string> v(w->m_stats.size());
			int i = 0;
			for (std::map<std::string, Stat>::const_iterator it = w->m_stats.begin(); it != w->m_stats.end(); ++it) {
				v[i++] = it->first;
			}
			return LuaHelpers::push(L, v);
		}

		int stats(lua_State *L) {
			// TODO
			return 0;
		}

		int linesAdded(lua_State *L) {
			std::string file = LuaHelpers::pops(L);
			if (w == NULL) return LuaHelpers::pushNil(L);
			if (w->m_stats.find(file) != w->m_stats.end()) {
				return LuaHelpers::push(L, w->m_stats[file].ladd);
			}
			return LuaHelpers::push(L, 0);
		}

		int bytesAdded(lua_State *L) {
			std::string file = LuaHelpers::pops(L);
			if (w == NULL) return LuaHelpers::pushNil(L);
			if (w->m_stats.find(file) != w->m_stats.end()) {
				return LuaHelpers::push(L, w->m_stats[file].cadd);
			}
			return LuaHelpers::push(L, 0);
		}

		int linesRemoved(lua_State *L) {
			std::string file = LuaHelpers::pops(L);
			if (w == NULL) return LuaHelpers::pushNil(L);
			if (w->m_stats.find(file) != w->m_stats.end()) {
				return LuaHelpers::push(L, w->m_stats[file].ldel);
			}
			return LuaHelpers::push(L, 0);
		}

		int bytesRemoved(lua_State *L) {
			std::string file = LuaHelpers::pops(L);
			if (w == NULL) return LuaHelpers::pushNil(L);
			if (w->m_stats.find(file) != w->m_stats.end()) {
				return LuaHelpers::push(L, w->m_stats[file].cdel);
			}
			return LuaHelpers::push(L, 0);
		}

	public:
		static const char className[];
		static Lunar<LuaDiffstat>::RegType methods[];

	public:
		Diffstat *w;
};

// Static variables for the lua bindings
const char LuaDiffstat::className[] = "diffstat";
Lunar<LuaDiffstat>::RegType LuaDiffstat::methods[] = {
	LUNAR_DECLARE_METHOD(LuaDiffstat, files),
	LUNAR_DECLARE_METHOD(LuaDiffstat, stats),
	LUNAR_DECLARE_METHOD(LuaDiffstat, linesAdded),
	LUNAR_DECLARE_METHOD(LuaDiffstat, bytesAdded),
	LUNAR_DECLARE_METHOD(LuaDiffstat, linesRemoved),
	LUNAR_DECLARE_METHOD(LuaDiffstat, bytesRemoved),
	{0,0}
};


#endif // LUADIFFSTAT_H_
