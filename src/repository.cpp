/*
 * pepper - SCM statistics report generator
 * Copyright (C) 2010 Jonas Gehring
 *
 * file: repository.cpp
 * Repository interface for report scripts
 */


#include "backend.h"
#include "options.h"

#include "luahelpers.h"

#include "repository.h"


// Static variables for the lua bindings
const char Repository::className[] = "Repository";
Lunar<Repository>::RegType Repository::methods[] = {
	LUNAR_DECLARE_METHOD(Repository, url),
	LUNAR_DECLARE_METHOD(Repository, type),
	LUNAR_DECLARE_METHOD(Repository, head),
	LUNAR_DECLARE_METHOD(Repository, branches),
	{0,0}
};


// Constructor
Repository::Repository(Backend *backend)
	: m_backend(backend)
{

}

// Default constructor for lua
Repository::Repository(lua_State *L)
	: m_backend(NULL)
{

}

// Destructor
Repository::~Repository()
{

}

// Returns the repository URL
int Repository::url(lua_State *L)
{
	return LuaHelpers::push(L, m_backend->options().repoUrl());
}

// Returns the repository type, i.e. the name of the current backend
int Repository::type(lua_State *L)
{
	return LuaHelpers::push(L, m_backend->name());
}

// Returns the current HEAD revision
int Repository::head(lua_State *L)
{
	std::string h;
	try {
		h = m_backend->head();
	} catch (const std::string &err) {
		return LuaHelpers::pushError(L, err);
	}
	return LuaHelpers::push(L, h);
}

// Returns a list of available branches (as a table)
int Repository::branches(lua_State *L)
{
	std::vector<std::string> b;
	try {
		b = m_backend->branches();
	} catch (const std::string &err) {
		return LuaHelpers::pushError(L, err);
	}
	return LuaHelpers::push(L, b);
}
