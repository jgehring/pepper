/*
 * pepper - SCM statistics report generator
 * Copyright (C) 2010 Jonas Gehring
 *
 * file: revision.cpp
 * Revision data class
 */


#include <iostream>

#include "luahelpers.h"
#include "bstream.h"

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


// Constructor
Revision::Revision(const std::string &id)
	: m_id(id), m_date(0)
{

}

// Constructor
Revision::Revision(const std::string &id, int64_t date, const std::string &author, const std::string &message, const Diffstat &diffstat)
	: m_id(id), m_date(date), m_author(author), m_message(message), m_diffstat(diffstat)
{

}

// Destructor
Revision::~Revision()
{

}

// Returns the revision ID (e.g., the revision number)
std::string Revision::id() const
{
	return m_id;
}

// Returns the diffstat object
Diffstat Revision::diffstat() const
{
	return m_diffstat;
}

// Writes the revision to a binary stream (not writing the ID)
void Revision::write(BOStream &out) const
{
	out << m_date << m_author << m_message;
	m_diffstat.write(out);
}

// Loads the revision from a binary stream (not changing the ID)
bool Revision::load(BIStream &in)
{
	in >> m_date >> m_author >> m_message;
	return m_diffstat.load(in);
}


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

// Constructor
LuaRevision::LuaRevision(Revision *r)
	: r(r)
{

}

// Default constructor for lua
LuaRevision::LuaRevision(lua_State *L)
	: r(NULL)
{

}

// Returns the revision ID (e.g., the revision number)
int LuaRevision::id(lua_State *L)
{
	return (r ? LuaHelpers::push(L, r->m_id) : LuaHelpers::pushNil(L));
}

// Returns the revision date, in seconds
int LuaRevision::date(lua_State *L)
{
	return (r ? LuaHelpers::push(L, r->m_date) : LuaHelpers::pushNil(L));
}

// Returns the author of this revision
int LuaRevision::author(lua_State *L)
{
	return (r ? LuaHelpers::push(L, r->m_author) : LuaHelpers::pushNil(L));
}

// Returns the message of this revision
int LuaRevision::message(lua_State *L)
{
	return (r ? LuaHelpers::push(L, r->m_message) : LuaHelpers::pushNil(L));
}

// Returns a diffstat for this revision
int LuaRevision::diffstat(lua_State *L)
{
	return (r ? LuaHelpers::push(L, &r->m_diffstat) : LuaHelpers::pushNil(L));
}
