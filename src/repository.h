/*
 * pepper - SCM statistics report generator
 * Copyright (C) 2010 Jonas Gehring
 *
 * file: repository.h
 * Repository interface (interface)
 */


#ifndef REPOSITORY_H_
#define REPOSITORY_H_


#include "lunar/lunar.h"


class Backend;


class Repository
{
	public:
		Repository(Backend *backend);
		~Repository();

		Backend *backend() const;

	private:
		Backend *m_backend;

	// Lua binding
	public:
		Repository(lua_State *L);

		int url(lua_State *L);
		int type(lua_State *L);
		int head(lua_State *L);
		int main_branch(lua_State *L);
		int branches(lua_State *L);

		static const char className[];
		static Lunar<Repository>::RegType methods[];
};


#endif // REPOSITORY_H_
