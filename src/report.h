/*
 * pepper - SCM statistics report generator
 * Copyright (C) 2010 Jonas Gehring
 *
 * file: report.h
 * Lua interface for gathering repository data (interface)
 */


#ifndef REPORT_H_
#define REPORT_H_


struct lua_State;


namespace Report
{

int openLib(lua_State *L);

} // namespace Report


#endif // REPORT_H_
