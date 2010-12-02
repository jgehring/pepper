/*
 * pepper - SCM statistics report generator
 * Copyright (C) 2010 Jonas Gehring
 *
 * file: utils.cpp
 * Miscellaneous utility functions
 */


#include <cstring>
#include <iostream>

#include <sys/time.h>

#include "main.h"

#ifdef HAVE_LIBZ
 #include <zlib.h>
#endif

#ifdef USE_POPEN_NOSHELL
 #include "popen-noshell/popen_noshell.h"
#endif

#include "utils.h"


namespace utils
{

// Wrapper for strptime()
int64_t ptime(const std::string &str, const std::string &format)
{
	struct tm tm;
	if (strptime(str.c_str(), format.c_str(), &tm) == NULL) {
		return -1;
	}
	return mktime(&tm);
}

// Removes white-space characters at the beginning and end of a string
void trim(std::string *str)
{
	int start = 0;
	int end = str->length()-1;

	while (start < end && isspace(str->at(start))) {
		++start;
	}
	while (end > start && isspace(str->at(end))) {
		--end;
	}

	if (start > 0 || end < (int)str->length()) {
		*str = str->substr(start, (end - start + 1));
	}
}

// Removes white-space characters at the beginning and end of a string
std::string trim(const std::string &str)
{
	std::string copy(str);
	trim(&copy);
	return copy;
}

// Split a string using the given token
std::vector<std::string> split(const std::string &str, const std::string &token, bool trim)
{
	std::vector<std::string> parts;
	size_t index = 0;

	if (token.length() == 0) {
		for (size_t i = 0; i < str.length(); i++) {
			parts.push_back(trim ? utils::trim(str.substr(i, 1)) : str.substr(i, 1));
		}
		return parts;
	}

	while (index < str.length()) {
		size_t pos = str.find(token, index);
		parts.push_back(trim ? utils::trim(str.substr(index, pos - index)) : str.substr(index, pos - index));
		if (pos == std::string::npos) {
			break;
		}
		index = pos + token.length();
		if (index == str.length()) {
			parts.push_back("");
		}
	}

	return parts;
}

// Joins several strings
std::string join(const std::vector<std::string> &v, const std::string &c)
{
	std::string res;
	for (unsigned int i = 0; i < v.size(); i++) {
		res += v[i];
		if (i < v.size()-1) {
			res += c;
		}
	}
	return res;
}

// sprintf for std::string
std::string strprintf(const char *format, ...)
{
	va_list vl;
	va_start(vl, format);

	std::ostringstream os;

	const char *ptr = format-1;
	while (*(++ptr) != '\0') {
		if (*ptr != '%') {
			os << *ptr;
			continue;
		}

		++ptr;

		// Only a subset of format specifiers is supported
		switch (*ptr) {
			case 'd':
			case 'i':
				os << va_arg(vl, int);
				break;

			case 'u':
				os << va_arg(vl, unsigned);
				break;

			case 'c':
				os << (unsigned char)va_arg(vl, int);
				break;

			case 'e':
			case 'E':
			case 'f':
			case 'F':
			case 'g':
			case 'G':
				os << va_arg(vl, double);
				break;

			case 's':
				os << va_arg(vl, const char *);
				break;

			case '%':
				os << '%';
				break;

			default:
#ifndef NDEBUG
				std::cerr << "Error in strprintf(): unknown format specifier " << *ptr << std::endl;
				exit(1);
#endif
				break;
		}
	}

	va_end(vl);
	return os.str();
}

// Pretty-prints a help screen option
void printOption(const std::string &option, const std::string &text)
{
	std::cout << "  " << option;
	if (option.length() < 30) {
		for (int i = option.length(); i < 32; i++) {
			std::cout << " ";
		}
	} else {
		std::cout << std::endl;
		for (int i = 0; i < 34; i++) {
			std::cout << " ";
		}
	}
	std::cout << text << std::endl;
}

// Runs the specified command line and returns the output
std::string execv(int *ret, const char * const *argv)
{
#ifdef USE_POPEN_NOSHELL
	struct popen_noshell_pass_to_pclose pclose_arg;
	FILE *pipe = popen_noshell(argv[0], argv, "r", &pclose_arg, 0);
#else
	// Concatenate arguments
	std::string cmd;
	const char * const *ptr = argv;
	while (*ptr) {
		cmd += *ptr;
		cmd += " ";
		++ptr;
	}
	FILE *pipe = popen(cmd.c_str(), "r");
#endif
	if (!pipe) {
		throw PEX(strprintf("Unable to open pipe for command %s", argv[0]));
	}

	char buffer[128];
	std::string result;
	while (!feof(pipe)) {
		if (fgets(buffer, 128, pipe) != NULL) {
			result += buffer;
		}
	}

#ifdef USE_POPEN_NOSHELL
	int r = pclose_noshell(&pclose_arg);
#else
	int r = pclose(pipe);
#endif
	if (ret != NULL) {
		*ret = r;
	}
	return result;
}


std::string exec(int *ret, const char *cmd, const char *arg1, const char *arg2, const char *arg3, const char *arg4, const char *arg5, const char *arg6, const char *arg7)
{
	const char **argv = new const char *[9];
	argv[0] = cmd;
	argv[1] = arg1; argv[2] = arg2; argv[3] = arg3;
	argv[4] = arg4; argv[5] = arg5; argv[6] = arg6;
	argv[7] = arg7; argv[8] = NULL;
	std::string out = execv(ret, argv);
	delete[] argv;
	return out;
}

inline uint32_t bswap(uint32_t source) {
	return 0
		| ((source & 0x000000ff) << 24) 
		| ((source & 0x0000ff00) << 8)
		| ((source & 0x00ff0000) >> 8)
		| ((source & 0xff000000) >> 24);
}

// Compresses the input data using zlib (or NULL)
std::vector<char> compress(const std::vector<char> &data, int level)
{
#ifdef HAVE_LIBZ
	unsigned long dlen = compressBound(data.size());
	std::vector<char> dest(dlen+4);

	// First four bytes: original data length
	uint32_t len = data.size();
#ifndef WORDS_BIGENDIAN
	len = bswap(len);
#endif
	memcpy((char *)&dest[0], (char *)&len, 4);

	int ret = ::compress2((unsigned char *)&dest[4], &dlen, reinterpret_cast<const unsigned char *>(&data[0]), data.size(), level);
	if (ret != Z_OK) {
		throw PEX(strprintf("Data compression failed (%d)", ret));
	}
	dest.resize(dlen+4);
	return dest;
#else
	return data;
#endif
}

// Decompresses the input data using zlib (or NULL)
std::vector<char> uncompress(const std::vector<char> &data)
{
#ifdef HAVE_LIBZ
	if (data.size() <= 4) {
		return std::vector<char>();
	}

	// Read original data size
	uint32_t dlen = 0;
	memcpy((char *)&dlen, (char *)&data[0], 4);
#ifndef WORDS_BIGENDIAN
	dlen = bswap(dlen);
#endif
	if (dlen == 0) {
		return std::vector<char>();
	}

	std::vector<char> dest(dlen);
	unsigned long ldlen = dlen;
	int ret = ::uncompress((unsigned char *)&dest[0], &ldlen, (const unsigned char *)&data[4], data.size()-4);
	if (ret != Z_OK) {
		throw PEX(strprintf("Corrupted data (%d)", ret));
	}
	return dest;
#else
	return data;
#endif
}

} // namespace utils
