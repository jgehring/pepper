/*
 * pepper - SCM statistics report generator
 * Copyright (C) 2010 Jonas Gehring
 *
 * file: luarevision.h
 * Lua bindings for the Revision class
 */


#ifndef LUAREVISION_H_
#define LUAREVISION_H_


#include "revision.h"
#include "luahelpers.h"
#include "luadiffstat.h"


// Lua wrapper for the revision class
class LuaRevision
{
	public:
		LuaRevision(Revision *rev) : w(rev), m_stat(&rev->m_diffstat) { }
		LuaRevision(lua_State *L) : w(NULL), m_stat(L) { }

		Revision *object() const { return w; }

		int id(lua_State *L) {
			return (w ? LuaHelpers::push(L, w->m_id) : LuaHelpers::pushNil(L));

		}
		int date(lua_State *L) {
			return (w ? LuaHelpers::push(L, w->m_date) : LuaHelpers::pushNil(L));
		}

		int author(lua_State *L) {
			return (w ? LuaHelpers::push(L, w->m_author) : LuaHelpers::pushNil(L));
		}

		int message(lua_State *L) {
			return (w ? LuaHelpers::push(L, w->m_message) : LuaHelpers::pushNil(L));
		}

		int diffstat(lua_State *L) {
			if (w == NULL) return LuaHelpers::pushNil(L);
			m_stat = LuaDiffstat(&w->m_diffstat);
			return LuaHelpers::push(L, &m_stat);
		}

	public:
		static const char className[];
		static Lunar<LuaRevision>::RegType methods[];

	private:
		Revision *w;
		LuaDiffstat m_stat;
};

// Static variables for the lua bindings
const char LuaRevision::className[] = "revision";
Lunar<LuaRevision>::RegType LuaRevision::methods[] = {
	LUNAR_DECLARE_METHOD(LuaRevision, id),
	LUNAR_DECLARE_METHOD(LuaRevision, date),
	LUNAR_DECLARE_METHOD(LuaRevision, author),
	LUNAR_DECLARE_METHOD(LuaRevision, message),
	LUNAR_DECLARE_METHOD(LuaRevision, diffstat),
	{0,0}
};


#endif // LUAREVISION_H_
