/*
 * pepper - SCM statistics report generator
 * Copyright (C) 2010-2014 Jonas Gehring
 *
 * Released under the GNU General Public License, version 3.
 * Please see the COPYING file in the source distribution for license
 * terms and conditions, or see http://www.gnu.org/licenses/.
 *
 * file: tag.cpp
 * Tag representation
 */


#include "main.h"

#include "luahelpers.h"

#include "tag.h"


// Empty constructor
Tag::Tag()
{

}

// Constructor
Tag::Tag(const std::string &id, const std::string &name)
	: m_id(id), m_name(name)
{

}

// Returns the tag ID
std::string Tag::id() const
{
	return m_id;
}

// Returns the tag name
std::string Tag::name() const
{
	return m_name;
}

// Compares tags by their name
bool Tag::operator<(const Tag &other) const
{
	return m_name < other.m_name;
}

/*
 * Lua binding
 */

const char Tag::className[] = "tag";
Lunar<Tag>::RegType Tag::methods[] = {
	LUNAR_DECLARE_METHOD(Tag, id),
	LUNAR_DECLARE_METHOD(Tag, name),
	{0,0}
};

Tag::Tag(lua_State *) {
}

int Tag::id(lua_State *L) {
	return LuaHelpers::push(L, m_id);
}

int Tag::name(lua_State *L) {
	return LuaHelpers::push(L, m_name);
}
