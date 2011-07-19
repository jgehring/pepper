/*
 * pepper - SCM statistics report generator
 * Copyright (C) 2010-2011 Jonas Gehring
 *
 * Released under the GNU General Public License, version 3.
 * Please see the COPYING file in the source distribution for license
 * terms and conditions, or see http://www.gnu.org/licenses/.
 *
 * file: utils.h
 * Miscellaneous utility functions
 */


#ifndef UTILS_H_
#define UTILS_H_


#include <string>
#include <vector>

#include "main.h"


namespace utils
{

std::vector<char> compress(const std::vector<char> &data, int level = 9);
std::vector<char> uncompress(const std::vector<char> &data);

inline std::string childId(const std::string &id) {
	size_t p = id.find_last_of(':');
	return (p != std::string::npos ? id.substr(p+1) : id);
}

uint32_t crc32(const char *data, size_t len);
inline uint32_t crc32(const std::vector<char> &data) {
	return crc32(&data[0], data.size());
}

} // namespace utils


#endif // UTILS_H_
