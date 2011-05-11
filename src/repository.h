/*
 * pepper - SCM statistics report generator
 * Copyright (C) 2010-2011 Jonas Gehring
 *
 * Released under the GNU General Public License, version 3.
 * Please see the COPYING file in the source distribution for license
 * terms and conditions, or see http://www.gnu.org/licenses/.
 *
 * file: repository.h
 * Repository interface (interface)
 */


#ifndef REPOSITORY_H_
#define REPOSITORY_H_


#include "lunar/lunar.h"

class Backend;
class RevisionIterator;


class Repository
{
	public:
		Repository(Backend *backend);
		~Repository();

		Backend *backend() const;

	private:
		Backend *m_backend;

	// Lua binding
	public:
		Repository(lua_State *L);

		int url(lua_State *L);
		int type(lua_State *L);
		int head(lua_State *L);
		int default_branch(lua_State *L);
		int branches(lua_State *L);
		int tags(lua_State *L);
		int tree(lua_State *L);
		int revision(lua_State *L);
		int iterator(lua_State *L);
		int cat(lua_State *L);

		// Compability methods
		int main_branch(lua_State *L);

		static const char className[];
		static Lunar<Repository>::RegType methods[];
};


#endif // REPOSITORY_H_
