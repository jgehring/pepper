/*
 * pepper - SCM statistics report generator
 * Copyright (C) 2010-2011 Jonas Gehring
 *
 * Released under the GNU General Public License, version 3.
 * Please see the COPYING file in the source distribution for license
 * terms and conditions, or see http://www.gnu.org/licenses/.
 *
 * file: luamodules.h
 * Extra C modules for the Lua API (interface)
 */


#ifndef LUAMODULES_H_
#define LUAMODULES_H_


struct lua_State *L;

namespace LuaModules {
	void registerModules(lua_State *L);
};


#endif // LUAMODULES_H_
