/*
 * pepper - SCM statistics report generator
 * Copyright (C) 2010 Jonas Gehring
 *
 * file: report.h
 * Lua interface for gathering repository data (interface)
 */


#ifndef REPORT_H_
#define REPORT_H_


class Backend;

struct lua_State;


namespace Report
{

int run(const char *script, Backend *backend);

} // namespace Report


#endif // REPORT_H_
