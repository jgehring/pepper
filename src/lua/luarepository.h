/*
 * pepper - SCM statistics report generator
 * Copyright (C) 2010 Jonas Gehring
 *
 * file: luarepository.h
 * Lua bindings for the Repository class 
 */


#ifndef LUAREPOSITORY_H_
#define LUAREPOSITORY_H_


#include "repository.h"
#include "options.h"
#include "luahelpers.h"


class LuaRepository
{
	public:
		LuaRepository(Repository *repo) : w(repo) { }
		LuaRepository(lua_State *L) : w(NULL) { }

		int url(lua_State *L) {
			return (w ? LuaHelpers::push(L, w->m_backend->options().repoUrl()) : LuaHelpers::pushNil(L));
		}

		int type(lua_State *L) {
			return (w ? LuaHelpers::push(L, w->m_backend->name()) : LuaHelpers::pushNil(L));
		}

		int head(lua_State *L) {
			if (w == NULL) return LuaHelpers::pushNil(L);
			std::string h;
			try {
				h = w->m_backend->head();
			} catch (const Pepper::Exception &ex) {
				return LuaHelpers::pushError(L, ex.what(), ex.what());
			}
			return LuaHelpers::push(L, h);
		}

		int main_branch(lua_State *L) {
			if (w == NULL) return LuaHelpers::pushNil(L);
			return LuaHelpers::push(L, w->m_backend->mainBranch());
		}

		int branches(lua_State *L) {
			if (w == NULL) return LuaHelpers::pushNil(L);
			std::vector<std::string> b;
			try {
				b = w->m_backend->branches();
			} catch (const Pepper::Exception &ex) {
				return LuaHelpers::pushError(L, ex.what(), ex.what());
			}
			return LuaHelpers::push(L, b);
		}

	public:
		static const char className[];
		static Lunar<LuaRepository>::RegType methods[];

	public:
		Repository *w;
};

// Static variables for the lua bindings
const char LuaRepository::className[] = "repository";
Lunar<LuaRepository>::RegType LuaRepository::methods[] = {
	LUNAR_DECLARE_METHOD(LuaRepository, url),
	LUNAR_DECLARE_METHOD(LuaRepository, type),
	LUNAR_DECLARE_METHOD(LuaRepository, head),
	LUNAR_DECLARE_METHOD(LuaRepository, main_branch),
	LUNAR_DECLARE_METHOD(LuaRepository, branches),
	{0,0}
};


#endif // LUAREPOSITORY_H_
