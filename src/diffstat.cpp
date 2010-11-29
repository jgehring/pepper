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

// Static diff parsing function
Diffstat DiffParser::parse(std::istream &in)
{
	std::string str, file;
	Diffstat ds;
	Diffstat::Stat stat;
	while (in.good()) {
		std::getline(in, str);
		if (!str.compare(0, 7, "Index: ")) {
			if (!file.empty()) {
				ds.m_stats[file] = stat;
			}
			stat = Diffstat::Stat();
			file = str.substr(7);
		} else if (!str.compare(0, 11, "diff --git ")) {
			if (!file.empty()) {
				ds.m_stats[file] = stat;
			}
			stat = Diffstat::Stat();
			std::vector<std::string> files = utils::split(str.substr(11), " b/");
			file = files[0].substr(2);
		} else if (!str.compare(0, 4, "====") || !str.compare(0, 4, "--- ") || !str.compare(0, 4, "+++ ")) {
			continue;
		} else if (str[0] == '-') {
			stat.cdel += str.length();
			++stat.ldel;
		} else if (str[0] == '+') {
			stat.cadd += str.length();
			++stat.ladd;
		}
	}
	if (!file.empty()) {
		ds.m_stats[file] = stat;
	}
	return ds;
}

// Main thread function
void DiffParser::run()
{
	m_stat = parse(m_in);
}
