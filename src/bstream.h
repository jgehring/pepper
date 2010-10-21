/*
 * pepper - SCM statistics report generator
 * Copyright (C) 2010 Jonas Gehring
 *
 * file: bstream.h
 * Binary input and output streams (interfaces)
 */


#ifndef BSTREAM_H_
#define BSTREAM_H_


#include <cstdio>
#include <string>

#include "main.h"


// Base class
class BStream
{
	public:
		BStream(FILE *f);
		~BStream();

		inline bool ok() const { return m_file != NULL; }
		inline bool eof() const { return feof(m_file); }
		inline size_t tell() const { return ftell(m_file); }
		inline bool seek(size_t offset) { return fseek(m_file, offset, SEEK_SET) == 0; }

	protected:
		// Byte swapping (from Qt)
		inline uint32_t bswap(uint32_t source) {
			return 0
				| ((source & 0x000000ff) << 24) 
				| ((source & 0x0000ff00) << 8)
				| ((source & 0x00ff0000) >> 8)
				| ((source & 0xff000000) >> 24);

		}
		inline uint64_t bswap(uint64_t source) {
			return 0
				| ((source & 0x00000000000000ffLL) << 56) 
				| ((source & 0x000000000000ff00LL) << 40) 
				| ((source & 0x0000000000ff0000LL) << 24) 
				| ((source & 0x00000000ff000000LL) << 8)
				| ((source & 0x000000ff00000000LL) >> 8)
				| ((source & 0x0000ff0000000000LL) >> 24) 
				| ((source & 0x00ff000000000000LL) >> 40) 
				| ((source & 0xff00000000000000LL) >> 56);
		}

	protected:
		FILE *m_file;
};

// Output stream
class BOStream : public BStream
{
	public:
		BOStream(const std::string &path, bool append = false);
		BOStream(FILE *f) : BStream(f) { }

		BOStream &operator<<(char c);
		BOStream &operator<<(uint32_t i);
		BOStream &operator<<(uint64_t i);
		BOStream &operator<<(const std::string &s);
};

// Input stream
class BIStream : public BStream
{
	public:
		BIStream(const std::string &path);
		BIStream(FILE *f) : BStream(f) { }

		BIStream &operator>>(char &c);
		BIStream &operator>>(uint32_t &i);
		BIStream &operator>>(uint64_t &i);
		BIStream &operator>>(std::string &s);
};


// Inlined functions
inline BOStream &BOStream::operator<<(char c) {
	fputc(c, m_file);
	return *this;
}

inline BOStream &BOStream::operator<<(uint32_t i) {
#ifndef WORDS_BIGENDIAN
	i = bswap(i);
#endif
	fwrite((char *)&i, 4, 1, m_file);
	return *this;
}

inline BOStream &BOStream::operator<<(uint64_t i) {
#ifndef WORDS_BIGENDIAN
	i = bswap(i);
#endif
	fwrite((char *)&i, 8, 1, m_file);
	return *this;
}

inline BOStream &BOStream::operator<<(const std::string &s) {
	fwrite(s.data(), 1, s.length(), m_file);
	fputc(0x00, m_file);
	return *this;
}

inline BIStream &BIStream::operator>>(char &c) {
	c = (char)fgetc(m_file);
	return *this;
}

inline BIStream &BIStream::operator>>(uint32_t &i) {
	fread((char *)&i, 4, 1, m_file);
#ifndef WORDS_BIGENDIAN
	i = bswap(i);
#endif
	return *this;
}

inline BIStream &BIStream::operator>>(uint64_t &i) {
	fread((char *)&i, 8, 1, m_file);
#ifndef WORDS_BIGENDIAN
	i = bswap(i);
#endif
	return *this;
}

inline BIStream &BIStream::operator>>(std::string &s) {
	char buffer[120], *bptr = buffer;
	int c;
	s.clear();
	while (true) {
		c = fgetc(m_file);
		switch (c) {
			case 0: break;
			case EOF: s.clear(); break;
			default: {
				*bptr = c;
				if (bptr-buffer == sizeof(buffer)) {
					s.append(buffer, sizeof(buffer));
					bptr = buffer;
				} else {
					++bptr;
				}
				continue;
			}
		}
		break;
	}

	s.append(buffer, bptr-buffer);
	return *this;
}


#endif // BSTREAM_H_
