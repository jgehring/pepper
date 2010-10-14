/*
 * pepper - SCM statistics report generator
 * Copyright (C) 2010 Jonas Gehring
 *
 * file: revision.h
 * Revision data class (interface)
 */


#ifndef REVISION_H_
#define REVISION_H_


#include <map>
#include <string>

#include "lunar.h"


class Revision
{
	public:
		Revision(const std::string &id);
		Revision(lua_State *L);
		~Revision();

		// Lua bindings
		int id(lua_State *L);
		int date(lua_State *L);
		int author(lua_State *L);

	public:
		static const char className[];
		static Lunar<Revision>::RegType methods[];

	private:
		std::string m_id;
		std::map<std::string, std::string> m_data;
};


#endif // REVISION_H_
