/*
 * pepper - SCM statistics report generator
 * Copyright (C) 2010-2011 Jonas Gehring
 *
 * Released under the GNU General Public License, version 3.
 * Please see the COPYING file in the source distribution for license
 * terms and conditions, or see http://www.gnu.org/licenses/.
 *
 * file: tests/units/backend.cpp
 * Program entry point for the backend test program
 */


#include "catch_runner.hpp"
#include "catch.hpp"

#ifdef CONIFG_H
 #include "config.h"
#endif

#include <zlib.h>


typedef std::map<std::string, std::string> stringmap;

namespace tutils
{

#if 0

// zlib decompression to temporary file
void gunzip(const std::string &src)
{
	mkstemp();
}
	
int inf(FILE *source, FILE *dest)
{
	int ret;
	unsigned have;
	z_stream strm;
	unsigned char in[CHUNK];
	unsigned char out[CHUNK];

	/* allocate inflate state */
	strm.zalloc = Z_NULL;
	strm.zfree = Z_NULL;
	strm.opaque = Z_NULL;
	strm.avail_in = 0;
	strm.next_in = Z_NULL;
	ret = inflateInit(&strm);
	if (ret != Z_OK)
		return ret;

	/* decompress until deflate stream ends or end of file */
	do {
		strm.avail_in = fread(in, 1, CHUNK, source);
		if (ferror(source)) {
			(void)inflateEnd(&strm);
			return Z_ERRNO;
		}
		if (strm.avail_in == 0)
			break;
		strm.next_in = in;

		/* run inflate() on input until output buffer not full */
		do {
			strm.avail_out = CHUNK;
			strm.next_out = out;
			ret = inflate(&strm, Z_NO_FLUSH);
			assert(ret != Z_STREAM_ERROR);  /* state not clobbered */
			switch (ret) {
				case Z_NEED_DICT:
					ret = Z_DATA_ERROR;     /* and fall through */
				case Z_DATA_ERROR:
				case Z_MEM_ERROR:
					(void)inflateEnd(&strm);
					return ret;
			}
			have = CHUNK - strm.avail_out;
			if (fwrite(out, 1, have, dest) != have || ferror(dest)) {
				(void)inflateEnd(&strm);
				return Z_ERRNO;
			}
		} while (strm.avail_out == 0);

		/* done when inflate() says it's done */
	} while (ret != Z_STREAM_END);

	/* clean up and return */
	(void)inflateEnd(&strm);
	return ret == Z_STREAM_END ? Z_OK : Z_DATA_ERROR;
}

#endif

} // namespace tutils


// Backend unit tests
#ifdef USE_SUBVERSION
 #include "test_subversion.h"
#endif


// Custom toString() implementations
namespace Catch
{
	template<>
	std::string toString<std::vector<std::string> >(const std::vector<std::string> &v)
	{
		std::ostringstream oss;
		oss << "std::vector( ";
		for (std::vector<std::string>::size_type i = 0; i < v.size(); i++) {
			oss << v[i];
			if (i < v.size()-1) oss << ", ";
		}
		oss << " )";
		return oss.str();
	}

	template<>
	std::string toString<std::vector<char> >(const std::vector<char> &v)
	{
		std::ostringstream oss;
		oss << "std::vector( ";
		oss << std::hex << std::showbase;
		for (std::vector<char>::size_type i = 0; i < v.size(); i++) {
			oss << (int)v[i];
			if (i < v.size()-1) oss << ", ";
		}
		oss << " )";
		return oss.str();
	}

	template<>
	std::string toString<stringmap>(const stringmap &v)
	{
		std::ostringstream oss;
		oss << "std::map( ";
		for (stringmap::const_iterator it = v.begin(); it != v.end(); ++it) {
			oss << it->first << " = " << it->second;

			stringmap::const_iterator jt(it);
			if (++jt != v.end()) oss << ", ";
		}
		oss << " )";
		return oss.str();
	}
}

// Program entry point
int main(int argc, char **argv)
{
	srand(time(NULL));
	return Catch::Main(argc, argv);
}
