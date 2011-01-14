/*
 * pepper - SCM statistics report generator
 * Copyright (C) 2010 Jonas Gehring
 *
 * file: diffstat.cpp
 * Diffstat object and parser
 */


#include <cstring>
#include <vector>

#include "bstream.h"
#include "luahelpers.h"
#include "utils.h"

#include "diffstat.h"



// Constructor
Diffstat::Diffstat()
{

}

// Destructor
Diffstat::~Diffstat()
{

}

// Returns the actual stats
std::map<std::string, Diffstat::Stat> Diffstat::stats() const
{
	return m_stats;
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

/*
 * Lua binding
 */

const char Diffstat::className[] = "diffstat";
Lunar<Diffstat>::RegType Diffstat::methods[] = {
	LUNAR_DECLARE_METHOD(Diffstat, files),
	LUNAR_DECLARE_METHOD(Diffstat, lines_added),
	LUNAR_DECLARE_METHOD(Diffstat, bytes_added),
	LUNAR_DECLARE_METHOD(Diffstat, lines_removed),
	LUNAR_DECLARE_METHOD(Diffstat, bytes_removed),
	{0,0}
};

Diffstat::Diffstat(lua_State *L) {
	Diffstat *other = Lunar<Diffstat>::check(L, 1);
	if (other == NULL) {
		return;
	}
	m_stats = other->m_stats;
}

int Diffstat::files(lua_State *L) {
	std::vector<std::string> v(m_stats.size());
	int i = 0;
	for (std::map<std::string, Stat>::const_iterator it = m_stats.begin(); it != m_stats.end(); ++it) {
		v[i++] = it->first;
	}
	return LuaHelpers::push(L, v);
}

int Diffstat::lines_added(lua_State *L) {
	if (lua_gettop(L) >= 1) {
		std::string file = LuaHelpers::pops(L);
		if (m_stats.find(file) != m_stats.end()) {
			return LuaHelpers::push(L, m_stats[file].ladd);
		}
		return LuaHelpers::push(L, 0);
	}

	// Return total
	int n = 0;
	for (std::map<std::string, Stat>::const_iterator it = m_stats.begin(); it != m_stats.end(); ++it) {
		n += it->second.ladd;
	}
	return LuaHelpers::push(L, n);
}

int Diffstat::bytes_added(lua_State *L) {
	if (lua_gettop(L) >= 1) {
		std::string file = LuaHelpers::pops(L);
		if (m_stats.find(file) != m_stats.end()) {
			return LuaHelpers::push(L, m_stats[file].cadd);
		}
		return LuaHelpers::push(L, 0);
	}

	// Return total
	int n = 0;
	for (std::map<std::string, Stat>::const_iterator it = m_stats.begin(); it != m_stats.end(); ++it) {
		n += it->second.cadd;
	}
	return LuaHelpers::push(L, n);
}

int Diffstat::lines_removed(lua_State *L) {
	if (lua_gettop(L) >= 1) {
		std::string file = LuaHelpers::pops(L);
		if (m_stats.find(file) != m_stats.end()) {
			return LuaHelpers::push(L, m_stats[file].ldel);
		}
		return LuaHelpers::push(L, 0);
	}

	// Return total
	int n = 0;
	for (std::map<std::string, Stat>::const_iterator it = m_stats.begin(); it != m_stats.end(); ++it) {
		n += it->second.ldel;
	}
	return LuaHelpers::push(L, n);
}

int Diffstat::bytes_removed(lua_State *L) {
	if (lua_gettop(L) >= 1) {
		std::string file = LuaHelpers::pops(L);
		if (m_stats.find(file) != m_stats.end()) {
			return LuaHelpers::push(L, m_stats[file].cdel);
		}
		return LuaHelpers::push(L, 0);
	}

	// Return total
	int n = 0;
	for (std::map<std::string, Stat>::const_iterator it = m_stats.begin(); it != m_stats.end(); ++it) {
		n += it->second.cdel;
	}
	return LuaHelpers::push(L, n);
}


// Constructor
DiffParser::DiffParser(std::istream &in)
	: sys::parallel::Thread(), m_in(in)
{

}

// Returns the internal diffstat object
Diffstat DiffParser::stat() const
{
	return m_stat;
}

// Static diff parsing function for unified diffs
Diffstat DiffParser::parse(std::istream &in)
{
	std::string str, file;
	Diffstat ds;
	Diffstat::Stat stat;
	int chunk[2] = {0, 0};
	while (in.good()) {
		std::getline(in, str);
		if (!str.compare(0, 4, "--- ") && chunk[0] <= 0 && chunk[0] <= 0) {
			if (!file.empty() && !stat.empty()) {
				ds.m_stats[file] = stat;
				file = std::string();
			}
			stat = Diffstat::Stat();
			std::vector<std::string> header = utils::split(str.substr(4), "\t");
			if (header.empty()) {
				throw PEX(std::string("EMPTY HEADER: ")+str);
			}
			if (header[0] != "/dev/null") {
				file = header[0];
				if (file[0] == '"' && file[file.length()-1] == '"') {
					file = file.substr(1, file.length()-2);
				}
				if (!file.compare(0, 2, "a/")) {
					file = file.substr(2);
				}
			}
		} else if (!str.compare(0, 4, "+++ ") && chunk[0] <= 0 && chunk[1] <= 0) {
			if (!file.empty() && !stat.empty()) {
				ds.m_stats[file] = stat;
				file = std::string();
			}
			stat = Diffstat::Stat();
			std::vector<std::string> header = utils::split(str.substr(4), "\t");
			if (header.empty()) {
				throw PEX(std::string("EMPTY HEADER: ")+str);
			}
			if (header[0] != "/dev/null") {
				file = header[0];
				if (file[0] == '"' && file[file.length()-1] == '"') {
					file = file.substr(1, file.length()-2);
				}
				if (!file.compare(0, 2, "b/")) {
					file = file.substr(2);
				}
			}
		} else if (!str.compare(0, 2, "@@")) {
			std::vector<std::string> header = utils::split(str.substr(2), "@@", true);
			if (header.empty()) {
				throw PEX(std::string("EMPTY HEADER: ")+str);
			}
			std::vector<std::string> ranges = utils::split(header[0], " ", true);
			if (ranges.size() < 2 || ranges[0].empty() || ranges[1].empty()) {
				throw PEX(std::string("EMPTY HEADER: ")+str);
			}
			size_t pos;
			if ((pos = ranges[0].find(',')) != std::string::npos) {
				utils::str2int(ranges[0].substr(pos+1), &chunk[(ranges[0][0] == '-' ? 0 : 1)]);
			} else {
				chunk[(ranges[0][0] == '-' ? 0 : 1)] = 1;
			}
			if ((pos = ranges[1].find(',')) != std::string::npos) {
				utils::str2int(ranges[1].substr(pos+1), &chunk[(ranges[1][0] == '-' ? 0 : 1)]);
			} else {
				chunk[(ranges[1][0] == '-' ? 0 : 1)] = 1;
			}
		} else if (str[0] == '-') {
			stat.cdel += str.length();
			++stat.ldel;
			--chunk[0];
		} else if (str[0] == '+') {
			stat.cadd += str.length();
			++stat.ladd;
			--chunk[1];
		} else {
			--chunk[0];
			--chunk[1];
		}
	}
	if (!file.empty() && !stat.empty()) {
		ds.m_stats[file] = stat;
	}
	return ds;
}

// Main thread function
void DiffParser::run()
{
	m_stat = parse(m_in);
}
