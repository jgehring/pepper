/*
 * pepper - SCM statistics report generator
 * Copyright (C) 2010 Jonas Gehring
 *
 * file: diffstat.cpp
 * Diffstat object
 */


#include <cstring>
#include <iostream>

#include "bstream.h"
#include "luahelpers.h"

#include "diffstat.h"


// Static variables for the lua bindings
const char Diffstat::className[] = "Diffstat";
Lunar<Diffstat>::RegType Diffstat::methods[] = {
	LUNAR_DECLARE_METHOD(Diffstat, files),
	LUNAR_DECLARE_METHOD(Diffstat, stats),
	LUNAR_DECLARE_METHOD(Diffstat, linesAdded),
	LUNAR_DECLARE_METHOD(Diffstat, bytesAdded),
	LUNAR_DECLARE_METHOD(Diffstat, linesRemoved),
	LUNAR_DECLARE_METHOD(Diffstat, bytesRemoved),
	{0,0}
};


// Constructor
Diffstat::Diffstat()
{

}

// Constructor
Diffstat::Diffstat(std::istream &in)
{
	parse(in);
}

// Default constructor for lua
Diffstat::Diffstat(lua_State *L)
{

}

// Destructor
Diffstat::~Diffstat()
{

}

// Writes the stat to a binary stream
void Diffstat::write(BOStream &out) const
{
	out << (uint32_t)m_stats.size();
	for (std::map<std::string, Stat>::const_iterator it = m_stats.begin(); it != m_stats.end(); ++it) {
		out << it->first.data();
		out << it->second.cadd << it->second.ladd << it->second.cdel << it->second.ldel;
	}
}

// Loads the stat from a binary stream
bool Diffstat::load(BIStream &in)
{
	m_stats.clear();
	uint32_t i = 0, n;
	in >> n;
	std::string buffer;
	Stat stat;
	while (i++ < n && !in.eof()) {
		in >> buffer;
		if (buffer.empty()) {
			return false;
		}
		in >> stat.cadd >> stat.ladd >> stat.cdel >> stat.ldel;
		m_stats[buffer] = stat;
	}
	return true;
}

// Returns the list of files that have been changed
int Diffstat::files(lua_State *L)
{
	std::vector<std::string> v(m_stats.size());
	int i = 0;
	for (std::map<std::string, Stat>::const_iterator it = m_stats.begin(); it != m_stats.end(); ++it) {
		v[i++] = it->first;
	}
	return LuaHelpers::push(L, v);
}

// Returns the list of stats
int Diffstat::stats(lua_State *L)
{
	// TODO
	return 0;
}

// Returns the number of lines added to the given file
int Diffstat::linesAdded(lua_State *L)
{
	std::string file = LuaHelpers::pop(L);
	if (m_stats.find(file) != m_stats.end()) {
		return LuaHelpers::push(L, m_stats[file].ladd);
	}
	return LuaHelpers::push(L, 0);
}

// Returns the number of bytes added to the given file
int Diffstat::bytesAdded(lua_State *L)
{
	std::string file = LuaHelpers::pop(L);
	if (m_stats.find(file) != m_stats.end()) {
		return LuaHelpers::push(L, m_stats[file].cadd);
	}
	return LuaHelpers::push(L, 0);
}

// Returns the number of lines removed from the given file
int Diffstat::linesRemoved(lua_State *L)
{
	std::string file = LuaHelpers::pop(L);
	if (m_stats.find(file) != m_stats.end()) {
		return LuaHelpers::push(L, m_stats[file].ldel);
	}
	return LuaHelpers::push(L, 0);
}

// Returns the number of bytes removed from the given file
int Diffstat::bytesRemoved(lua_State *L)
{
	std::string file = LuaHelpers::pop(L);
	if (m_stats.find(file) != m_stats.end()) {
		return LuaHelpers::push(L, m_stats[file].cdel);
	}
	return LuaHelpers::push(L, 0);
}

// Parses output generated from diff
void Diffstat::parse(std::istream &in)
{
	std::string str, file;
	Stat stat;
	while (in.good()) {
		std::getline(in, str);
		if (!str.compare(0, 7, "Index: ")) {
			if (!file.empty()) {
				m_stats[file] = stat;
			}
			stat = Stat();
			file = str.substr(7);
		} else if (!str.compare(0, 4, "====") || !str.compare(0, 4, "--- ") || !str.compare(0, 4, "+++ ")) {
			continue;
		} else if (str[0] == '-') {
			stat.cdel += str.length()-1;
			++stat.ldel;
		} else if (str[0] == '+') {
			stat.cadd += str.length()-1;
			++stat.ladd;
		}
	}
	if (!file.empty()) {
		m_stats[file] = stat;
	}
}
