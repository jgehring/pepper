/*
 * pepper - SCM statistics report generator
 * Copyright (C) 2010 Jonas Gehring
 *
 * file: report.h
 * Lua interface for gathering repository data (interface)
 */


#ifndef REPORT_H_
#define REPORT_H_


#include <string>


class Backend;

struct lua_State;


namespace Report
{

int run(const std::string &script, Backend *backend);

void printHelp(const std::string &script);

} // namespace Report


#endif // REPORT_H_
