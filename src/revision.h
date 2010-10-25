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

#include "main.h"

#include "lunar.h"

#include "diffstat.h"


class Revision
{
	public:
		__attribute__((deprecated)) Revision(const std::string &id, const Diffstat &diffstat);
		Revision(const std::string &id, int64_t date, const std::string &author, const std::string &message, const Diffstat &diffstat);
		Revision(lua_State *L);
		~Revision();

		std::string idString() const;

		// Lua bindings
		int id(lua_State *L);
		int date(lua_State *L);
		int author(lua_State *L);
		int message(lua_State *L);
		int diffstat(lua_State *L);

	public:
		static const char className[];
		static Lunar<Revision>::RegType methods[];

	private:
		std::string m_id;
		int64_t m_date;
		std::string m_author;
		std::string m_message;
		Diffstat m_diffstat;
};


#endif // REVISION_H_
