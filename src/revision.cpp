/*
 * pepper - SCM statistics report generator
 * Copyright (C) 2010-2014 Jonas Gehring
 *
 * Released under the GNU General Public License, version 3.
 * Please see the COPYING file in the source distribution for license
 * terms and conditions, or see http://www.gnu.org/licenses/.
 *
 * file: revision.cpp
 * Revision data class
 */


#include "main.h"

#include "bstream.h"
#include "logger.h"
#include "luahelpers.h"
#include "strlib.h"

#include "revision.h"


// Constructor
Revision::Revision(const std::string &id)
	: m_id(id), m_date(0), m_diffstat(std::make_shared<Diffstat>())
{

}

// Constructor
Revision::Revision(const std::string &id, int64_t date, const std::string &author, const std::string &message, DiffstatPtr diffstat)
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
DiffstatPtr Revision::diffstat() const
{
	return m_diffstat;
}

// Writes the revision to a binary stream (not writing the ID)
void Revision::write(BOStream &out) const
{
	out << 'R' << char(1); // Head and version
	out << m_date << m_author << m_message;
	m_diffstat->write(out);
	out << 'V'; // Tail
}

// Loads the revision from a binary stream (not changing the ID)
bool Revision::load(BIStream &in)
{
	char c, v;
	in >> c;
	if (c != 'R') { // Head
		return false;
	}
	in >> v;
	if (v != 1) {
		PDEBUG << "Unknown version number " << int(v) << ", aborting" << endl;
		return false;
	}

	in >> m_date >> m_author >> m_message;
	if (!m_diffstat->load(in)) {
		return false;
	}

	in >> c;
	if (c != 'V') { // Tail
		return false;
	}
	return in.ok();
}

// Writes the revision to a binary stream (not writing the ID)
void Revision::write03(BOStream &out) const
{
	out << m_date << m_author << m_message;
	m_diffstat->write(out);
}

// Loads the revision from a binary stream (not changing the ID)
bool Revision::load03(BIStream &in)
{
	in >> m_date >> m_author >> m_message;
	if (!m_diffstat->load(in)) {
		return false;
	}
	return in.ok();
}

/*
 * Lua binding
 */

const char Revision::className[] = "revision";
Lunar<Revision>::RegType Revision::methods[] = {
	LUNAR_DECLARE_METHOD(Revision, id),
	LUNAR_DECLARE_METHOD(Revision, parent_id),
	LUNAR_DECLARE_METHOD(Revision, date),
	LUNAR_DECLARE_METHOD(Revision, author),
	LUNAR_DECLARE_METHOD(Revision, message),
	LUNAR_DECLARE_METHOD(Revision, diffstat),
	{0,0}
};

Revision::Revision(lua_State *) {
}

int Revision::id(lua_State *L) {
	return LuaHelpers::push(L, str::split(m_id, ":").back());
}

int Revision::parent_id(lua_State *L) {
	std::vector<std::string> ids = str::split(m_id, ":");
	if (ids.size() > 1) {
		return LuaHelpers::push(L, ids.front());
	}
	return LuaHelpers::pushNil(L);
}

int Revision::date(lua_State *L) {
	return LuaHelpers::push(L, m_date);
}

int Revision::author(lua_State *L) {
	return LuaHelpers::push(L, m_author);
}

int Revision::message(lua_State *L) {
	return LuaHelpers::push(L, m_message);
}

int Revision::diffstat(lua_State *L) {
	return LuaHelpers::push(L, m_diffstat);
}
