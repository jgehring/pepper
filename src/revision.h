/*
 * pepper - SCM statistics report generator
 * Copyright (C) 2010-2011 Jonas Gehring
 *
 * Released under the GNU General Public License, version 3.
 * Please see the COPYING file in the source distribution for license
 * terms and conditions, or see http://www.gnu.org/licenses/.
 *
 * file: revision.h
 * Revision data class (interface)
 */


#ifndef REVISION_H_
#define REVISION_H_


#include <string>

#include "main.h"

#include "diffstat.h"

#include "lunar/lunar.h"

class BIStream;
class BOStream;


class Revision
{
	friend class Repository;
	friend class RevisionIterator;

	public:
		Revision(const std::string &id);
		Revision(const std::string &id, int64_t date, const std::string &author, const std::string &message, const Diffstat &diffstat);
		~Revision();

		std::string id() const;
		Diffstat diffstat() const;

		void write(BOStream &out) const;
		bool load(BIStream &in);

	PEPPER_PVARS:
		std::string m_id;
		int64_t m_date;
		std::string m_author;
		std::string m_message;
		Diffstat m_diffstat;

	// Lua binding
	public:
		Revision(lua_State *L);

		int id(lua_State *L);
		int parent_id(lua_State *L);
		int date(lua_State *L);
		int author(lua_State *L);
		int message(lua_State *L);
		int diffstat(lua_State *L);

		static const char className[];
		static Lunar<Revision>::RegType methods[];
};


#endif // REVISION_H_
