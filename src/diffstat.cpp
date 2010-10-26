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

#include "diffstat.h"



// Constructor
Diffstat::Diffstat()
{

}

// Constructor
Diffstat::Diffstat(std::istream &in)
{
	parse(in);
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
