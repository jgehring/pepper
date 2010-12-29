/*
 * pepper - SCM statistics report generator
 * Copyright (C) 2010 Jonas Gehring
 *
 * file: tag.h
 * Tag representation (interface)
 */


#ifndef TAG_H_
#define TAG_H_


#include <string>

#include "main.h"

#include "lunar/lunar.h"


class Tag
{
	public:
		Tag();
		Tag(const std::string &id, const std::string &name);

		std::string id() const;
		std::string name() const;

	private:
		std::string m_id;
		std::string m_name;

	// Lua binding
	public:
		Tag(lua_State *L);

		int id(lua_State *L);
		int name(lua_State *L);

		static const char className[];
		static Lunar<Tag>::RegType methods[];
};


#endif // TAG_H_
