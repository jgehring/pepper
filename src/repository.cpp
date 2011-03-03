/*
 * pepper - SCM statistics report generator
 * Copyright (C) 2010-2011 Jonas Gehring
 *
 * Released under the GNU General Public License, version 3.
 * Please see the COPYING file in the source distribution for license
 * terms and conditions, or see http://www.gnu.org/licenses/.
 *
 * file: repository.cpp
 * Repository interface for report scripts
 */


#include "main.h"

#include "backend.h"
#include "logger.h"
#include "luahelpers.h"
#include "options.h"
#include "revision.h"
#include "revisioniterator.h"
#include "tag.h"

#include "repository.h"


// Constructor
Repository::Repository(Backend *backend)
	: m_backend(backend)
{

}

// Destructor
Repository::~Repository()
{

}

// Returns the backend
Backend *Repository::backend() const
{
	return m_backend;
}

/*
 * Lua binding
 */

const char Repository::className[] = "repository";
Lunar<Repository>::RegType Repository::methods[] = {
	LUNAR_DECLARE_METHOD(Repository, url),
	LUNAR_DECLARE_METHOD(Repository, type),
	LUNAR_DECLARE_METHOD(Repository, head),
	LUNAR_DECLARE_METHOD(Repository, branches),
	LUNAR_DECLARE_METHOD(Repository, default_branch),
	LUNAR_DECLARE_METHOD(Repository, tags),
	LUNAR_DECLARE_METHOD(Repository, tree),
	LUNAR_DECLARE_METHOD(Repository, revision),
	LUNAR_DECLARE_METHOD(Repository, iterator),

	LUNAR_DECLARE_METHOD(Repository, main_branch),
	{0,0}
};

Repository::Repository(lua_State *)
{
	m_backend = NULL;
}

int Repository::url(lua_State *L)
{
	return (m_backend ? LuaHelpers::push(L, m_backend->options().repository()) : LuaHelpers::pushNil(L));
}

int Repository::type(lua_State *L)
{
	return (m_backend ? LuaHelpers::push(L, m_backend->name()) : LuaHelpers::pushNil(L));
}

int Repository::head(lua_State *L)
{
	if (m_backend == NULL) return LuaHelpers::pushNil(L);
	std::string branch;
	if (lua_gettop(L) > 0) {
		branch = LuaHelpers::pops(L);
	}
	std::string h;
	try {
		h = m_backend->head(branch);
	} catch (const PepperException &ex) {
		return LuaHelpers::pushError(L, ex.what(), ex.where());
	}
	return LuaHelpers::push(L, h);
}

int Repository::default_branch(lua_State *L)
{
	if (m_backend == NULL) return LuaHelpers::pushNil(L);
	return LuaHelpers::push(L, m_backend->mainBranch());
}

int Repository::branches(lua_State *L)
{
	if (m_backend == NULL) return LuaHelpers::pushNil(L);
	std::vector<std::string> b;
	try {
		b = m_backend->branches();
	} catch (const PepperException &ex) {
		return LuaHelpers::pushError(L, ex.what(), ex.where());
	}
	return LuaHelpers::push(L, b);
}

int Repository::tags(lua_State *L)
{
	if (m_backend == NULL) return LuaHelpers::pushNil(L);
	std::vector<Tag> t;
	try {
		t = m_backend->tags();
	} catch (const PepperException &ex) {
		return LuaHelpers::pushError(L, ex.what(), ex.where());
	}

	lua_createtable(L, t.size(), 0);
	int table = lua_gettop(L);
	for (unsigned int i = 0; i < t.size(); i++) {
		Lunar<Tag>::push(L, new Tag(t[i]), true);
		lua_rawseti(L, table, i+1);
	}
	return 1;
}

int Repository::tree(lua_State *L)
{
	if (m_backend == NULL) return LuaHelpers::pushNil(L);

	std::string id = LuaHelpers::pops(L);
	std::vector<std::string> t;
	try {
		t = m_backend->tree(id);
	} catch (const PepperException &ex) {
		return LuaHelpers::pushError(L, ex.what(), ex.where());
	}
	return LuaHelpers::push(L, t);
}

int Repository::revision(lua_State *L)
{
	if (m_backend == NULL) return LuaHelpers::pushNil(L);

	std::string id = LuaHelpers::pops(L);
	Revision *rev = NULL;
	try {
		rev = m_backend->revision(id);
	} catch (const PepperException &ex) {
		return LuaHelpers::pushError(L, ex.what(), ex.where());
	}
	return LuaHelpers::push(L, rev); // TODO: Memory leak!
}

int Repository::iterator(lua_State *L)
{
	if (lua_gettop(L) < 1 || lua_gettop(L) > 3) {
		return luaL_error(L, "Invalid number of arguments (1 to 3 expected)");
	}

	std::string branch;
	int64_t start = -1, end = -1;
	switch (lua_gettop(L)) {
		case 3: end = LuaHelpers::popi(L);
		case 2: start = LuaHelpers::popi(L);
		case 1: branch = LuaHelpers::pops(L);
		default: break;
	}

	RevisionIterator *it = NULL;
	try {
		it = new RevisionIterator(m_backend, branch, start, end);
	} catch (const PepperException &ex) {
		return LuaHelpers::pushError(L, ex.what(), ex.where());
	}
	return LuaHelpers::push(L, it);
}

int Repository::main_branch(lua_State *L)
{
	return default_branch(L);
}
