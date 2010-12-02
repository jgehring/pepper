/*
 * pepper - SCM statistics report generator
 * Copyright (C) 2010 Jonas Gehring
 *
 * file: tests/diffstat/main.cpp
 * Short diffstat program using the built-in diffstat code
 */


#include <iostream>

#include "diffstat.h"


// Program entry point
int main(int, char **)
{
	std::map<std::string, Diffstat::Stat> stat = DiffParser::parse(std::cin).stats();

	uint64_t ins = 0, del = 0;
	for (std::map<std::string, Diffstat::Stat>::const_iterator it = stat.begin(); it != stat.end(); ++it) {
		std::cout << it->second.ladd << "," << it->second.ldel << ",0," << it->first << std::endl;
		ins += it->second.ladd;
		del += it->second.ldel;
	}

	// Print a summary line similar to the last line of the diffstat command
	// This should be equal to "diffstat | tail -n 1"
	std::cout << " " << stat.size() << " files changed,";
	std::cout << " " << ins << " insertions(+),";
	std::cout << " " << del << " deletions(-)" << std::endl;
	return 0;
}
