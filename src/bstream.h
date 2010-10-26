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

#ifdef HAVE_LIBZ
 #include <zlib.h>
#endif


// Base class
class BStream
{
	public:
#ifdef HAVE_LIBZ
		typedef gzFile FilePtr;
#else
		typedef FILE *FilePtr;
#endif

	public:
		BStream(FilePtr f);
		~BStream();

		inline bool ok() const { return m_file != NULL; }
		inline bool eof() const {
#ifdef HAVE_LIBZ
			return gzeof(m_file);
#else
			return feof(m_file);
#endif
		}
		inline size_t tell() const {
#ifdef HAVE_LIBZ
			return gztell(m_file);
#else
			return ftell(m_file);
#endif
		}
		inline bool seek(size_t offset) {
#ifdef HAVE_LIBZ
			return gzseek(m_file, offset, SEEK_SET) >= 0;
#else
			return fseek(m_file, offset, SEEK_SET) == 0;
#endif
		}

	protected:
		// Raw data I/O
		inline int read(void *ptr, size_t n) {
#ifdef HAVE_LIBZ
			return gzread(m_file, ptr, n);
#else
			return fread(ptr, 1, n, m_file);
#endif
		}
		inline int write(const void *ptr, size_t n) {
#ifdef HAVE_LIBZ
			return gzwrite(m_file, ptr, n);
#else
			return fwrite(ptr, 1, n, m_file);
#endif
		}

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
		FilePtr m_file;
};

// Output stream
class BOStream : public BStream
{
	public:
		BOStream(const std::string &path, bool append = false);
		BOStream(FilePtr f) : BStream(f) { }

		BOStream &operator<<(char c);
		BOStream &operator<<(uint32_t i);
		BOStream &operator<<(uint64_t i);
		inline BOStream &operator<<(int64_t i) { return (*this << static_cast<uint64_t>(i)); }
		BOStream &operator<<(const std::string &s);
};

// Input stream
class BIStream : public BStream
{
	public:
		BIStream(const std::string &path);
		BIStream(FilePtr f) : BStream(f) { }

		BIStream &operator>>(char &c);
		BIStream &operator>>(uint32_t &i);
		BIStream &operator>>(uint64_t &i);
		inline BIStream &operator>>(int64_t &i) { return (*this >> reinterpret_cast<uint64_t &>(i)); }
		BIStream &operator>>(std::string &s);
};


// Inlined functions
inline BOStream &BOStream::operator<<(char c) {
#ifdef HAVE_LIBZ
	gzputc(m_file, c);
#else
	fputc(c, m_file);
#endif
	return *this;
}

inline BOStream &BOStream::operator<<(uint32_t i) {
#ifndef WORDS_BIGENDIAN
	i = bswap(i);
#endif
	write((char *)&i, 4);
	return *this;
}

inline BOStream &BOStream::operator<<(uint64_t i) {
#ifndef WORDS_BIGENDIAN
	i = bswap(i);
#endif
	write((char *)&i, 8);
	return *this;
}

inline BOStream &BOStream::operator<<(const std::string &s) {
	write(s.data(), s.length());
	return (*this << '\0');
}

inline BIStream &BIStream::operator>>(char &c) {
#ifdef HAVE_LIBZ
	c = (char)gzgetc(m_file);
#else
	c = (char)fgetc(m_file);
#endif
	return *this;
}

inline BIStream &BIStream::operator>>(uint32_t &i) {
	read((char *)&i, 4);
#ifndef WORDS_BIGENDIAN
	i = bswap(i);
#endif
	return *this;
}

inline BIStream &BIStream::operator>>(uint64_t &i) {
	read((char *)&i, 8);
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
#ifdef HAVE_LIBZ
		c = gzgetc(m_file);
#else
		c = fgetc(m_file);
#endif
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
