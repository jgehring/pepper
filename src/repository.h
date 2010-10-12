/*
 * pepper - SCM statistics report generator
 * Copyright (C) 2010 Jonas Gehring
 *
 * file: repository.h
 * Repository interface for report scripts (interface)
 */


#ifndef REPOSITORY_H_
#define REPOSITORY_H_


#include "lunar.h"

class Backend;


class Repository
{
	public:
		Repository(Backend *backend);
		Repository(lua_State *L);
		~Repository();

		int url(lua_State *L);
		int type(lua_State *L);
		int head(lua_State *L);
		int branches(lua_State *L);

	public:
		static const char className[];
		static Lunar<Repository>::RegType methods[];

	private:
		Backend *m_backend;
};


#endif // REPOSITORY_H_
