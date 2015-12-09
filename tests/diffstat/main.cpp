/*
 * pepper - SCM statistics report generator
 * Copyright (C) 2010-present Jonas Gehring
 *
 * Released under the GNU General Public License, version 3.
 * Please see the COPYING file in the source distribution for license
 * terms and conditions, or see http://www.gnu.org/licenses/.
 *
 * file: tests/diffstat/main.cpp
 * Short diffstat program using the built-in diffstat code
 */


#include <iostream>

#include "diffstat.h"


// Program entry point
int main(int, char **)
{
	std::map<std::string, Diffstat::Stat> stat = DiffParser::parse(std::cin)->stats();

	uint64_t ins = 0, del = 0;
	for (std::map<std::string, Diffstat::Stat>::const_iterator it = stat.begin(); it != stat.end(); ++it) {
		ins += it->second.ladd;
		del += it->second.ldel;
	}

	// Print a summary line similar to the last line of the diffstat command
	// This should be equal to "diffstat | tail -n 1"
	std::cout << " " << stat.size() << " file" << (stat.size() != 1 ? "s" : "") << " changed";
	if (ins > 0) {
		std::cout << ", " << ins << (ins > 1 ? " insertions(+)" : " insertion(+)");
	}
	if (del > 0) {
		std::cout << ", " << del << (del > 1 ? " deletions(-)" : " deletion(-)");
	}
	std::cout << std::endl;
	return 0;
}
