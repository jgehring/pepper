/*
 * pepper - SCM statistics report generator
 * Copyright (C) 2010 Jonas Gehring
 *
 * file: repository.cpp
 * Repository interface for report scripts
 */


#include "backend.h"

#include "repository.h"


// Static variables for the lua bindings
const char Repository::className[] = "Repository";
Lunar<Repository>::RegType Repository::methods[] = {
	LUNAR_DECLARE_METHOD(Repository, type),
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

// Returns the repository type, i.e. the name of the current backend
int Repository::type(lua_State *L)
{
	lua_pushstring(L, m_backend->name().c_str());
	return 1;
}
