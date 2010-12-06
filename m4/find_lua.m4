dnl
dnl pepper - SCM statistics report generator
dnl Copyright (C) 2010 Jonas Gehring
dnl
dnl Check for lua using, inspired by VLC's configure.ac
dnl


AC_DEFUN([FIND_LUA], [
	PKG_CHECK_MODULES([LUA], [lua5.1], [found_lua="yes"], [
		PKG_CHECK_MODULES([LUA], [lua-5.1], [found_lua="yes"], [
			AC_MSG_WARN([lua5.1 not found, trying lua >= 5.1 instead])
			PKG_CHECK_MODULES([LUA], [lua >= 5.1], [found_lua="yes"], [
				dnl Fall back to manual configuration
				found_lua="yes"
				AC_CHECK_HEADERS([lua.h lauxlib.h lualib.h], [], [found_lua="no"])
				AC_CHECK_LIB([lua5.1], [luaL_newstate], [LUA_LIBS="-llua5.1"], [
					AC_CHECK_LIB([lua51], [luaL_newstate], [LUA_LIBS="-llua51"], [
						AC_CHECK_LIB(lua , luaL_newstate, [LUA_LIBS="-llua"], [lua_found="no"], [-lm])
					])
				])
			])
		])
	])
])
