/*
 * pepper - SCM statistics report generator
 * Copyright (C) 2010-2014 Jonas Gehring
 *
 * Released under the GNU General Public License, version 3.
 * Please see the COPYING file in the source distribution for license
 * terms and conditions, or see http://www.gnu.org/licenses/.
 *
 * file: tag.h
 * Tag representation (interface)
 */


#ifndef TAG_H_
#define TAG_H_


#include <string>

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
