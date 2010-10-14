/*
 * pepper - SCM statistics report generator
 * Copyright (C) 2010 Jonas Gehring
 *
 * file: revision.cpp
 * Revision data class
 */


#include "luahelpers.h"

#include "revision.h"


// Utiliy function for getting values out of a map without inserting new ones
static inline std::string mapget(const std::map<std::string, std::string> &map, const std::string &key)
{
	std::map<std::string, std::string>::const_iterator it = map.find(key);
	if (it != map.end()) {
		return (*it).second;
	}
	return std::string();
}


// Static variables for the lua bindings
const char Revision::className[] = "Revision";
Lunar<Revision>::RegType Revision::methods[] = {
	LUNAR_DECLARE_METHOD(Revision, id),
	LUNAR_DECLARE_METHOD(Revision, date),
	LUNAR_DECLARE_METHOD(Revision, author),
	{0,0}
};


// Constructor
Revision::Revision(const std::string &id)
	: m_id(id)
{

}

// Default constructor for lua
Revision::Revision(lua_State *L)
{

}

// Destructor
Revision::~Revision()
{

}

// Returns the revision ID (e.g., the revision number)
int Revision::id(lua_State *L)
{
	return LuaHelpers::push(L, m_id);
}

int Revision::date(lua_State *L)
{
	return LuaHelpers::push(L, mapget(m_data, "date"));
}

int Revision::author(lua_State *L)
{
	return LuaHelpers::push(L, mapget(m_data, "author"));
}
