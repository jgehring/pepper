/*
 * pepper - SCM statistics report generator
 * Copyright (C) 2010-2014 Jonas Gehring
 *
 * Released under the GNU General Public License, version 3.
 * Please see the COPYING file in the source distribution for license
 * terms and conditions, or see http://www.gnu.org/licenses/.
 *
 * file: bstream.h
 * Binary input and output streams (interfaces)
 */


#ifndef BSTREAM_H_
#define BSTREAM_H_


#include <cassert>
#include <cstdio>
#include <string>
#include <vector>

#include "main.h"


// read()/write() macros with assertions
#if defined(NDEBUG) || 1
 // Don't use assertions, since realiable EOF testing requires
 // stream operators to fail silently.
 #define ASSERT_READ(p, n) do { read(p, n); } while (0)
 #define ASSERT_WRITE(p, n) do { write(p, n); } while (0)
#else
 #define ASSERT_READ(p, n) do { assert(read(p, n) == (ssize_t)n); } while (0)
 #define ASSERT_WRITE(p, n) do { assert(write(p, n) == (ssize_t)n); } while (0)
#endif

// Base class for all streams
class BStream
{
	public:
		// Abstract base class for raw streams
		class RawStream
		{
			public:
				RawStream() { }
				virtual ~RawStream() { }

				virtual bool ok() const = 0;
				virtual bool eof() const = 0;
				virtual size_t tell() const = 0;
				virtual bool seek(size_t offset) = 0;
				virtual ssize_t read(void *ptr, size_t n) = 0;
				virtual ssize_t write(const void *ptr, size_t n) = 0;
		};

		BStream(RawStream *stream) : m_stream(stream) { }
		virtual ~BStream() { delete m_stream; }

		inline bool ok() const { return m_stream != NULL && m_stream->ok(); }
		inline bool eof() const { return m_stream == NULL || m_stream->eof(); }
		inline size_t tell() const { return (m_stream ? m_stream->tell() : 0); }
		inline bool seek(size_t offset) { return (m_stream ? m_stream->seek(offset) : false); }
		inline ssize_t read(void *ptr, size_t n) { return (m_stream ? m_stream->read(ptr, n) : 0); }
		inline ssize_t write(const void *ptr, size_t n) { return (m_stream ? m_stream->write(ptr, n) : 0); }

		// Byte swapping (from Qt)
		static inline uint32_t bswap(uint32_t source) {
			return 0
				| ((source & 0x000000ff) << 24) 
				| ((source & 0x0000ff00) << 8)
				| ((source & 0x00ff0000) >> 8)
				| ((source & 0xff000000) >> 24);
		}
		static inline uint64_t bswap(uint64_t source) {
			char *t = (char *)&source;
			std::swap(t[0], t[7]), std::swap(t[1], t[6]);
			std::swap(t[2], t[5]), std::swap(t[3], t[4]);
			return source;
		}

	protected:
		RawStream *m_stream;
};

// Input stream
class BIStream : public BStream
{
	public:
		BIStream(const std::string &path);
		BIStream(FILE *f);

		BIStream &operator>>(char &c);
		BIStream &operator>>(uint32_t &i);
		BIStream &operator>>(uint64_t &i);
		inline BIStream &operator>>(int64_t &i) { return (*this >> reinterpret_cast<uint64_t &>(i)); }
		BIStream &operator>>(std::string &s);
		BIStream &operator>>(std::vector<char> &v);
		template<typename T> BIStream &operator>>(std::vector<T> &v) {
			uint32_t size;
			(*this) >> size;
			v.resize(size);
			for (uint32_t i = 0; i < size && !eof(); i++) {
				(*this) >> v[i];
			}
			return *this;
		}

	protected:
		BIStream(RawStream *stream);
};

// Output stream
class BOStream : public BStream
{
	public:
		BOStream(const std::string &path, bool append = false);
		BOStream(FILE *f);

		BOStream &operator<<(char c);
		BOStream &operator<<(uint32_t i);
		BOStream &operator<<(uint64_t i);
		inline BOStream &operator<<(int64_t i) { return (*this << static_cast<uint64_t>(i)); }
		BOStream &operator<<(const std::string &s);
		BOStream &operator<<(const std::vector<char> &v);
		template<typename T> BOStream &operator<<(const std::vector<T> &v) {
			(*this) << (uint32_t)v.size();
			for (uint32_t i = 0; i < v.size(); i++) {
				(*this) << v[i];
			}
			return *this;
		}

	protected:
		BOStream(RawStream *stream);
};

// Memory input stream
class MIStream : public BIStream
{
	public:
		MIStream(const char *data, size_t n);
		MIStream(const std::vector<char> &data);
};

// Memory output stream
class MOStream : public BOStream
{
	public:
		MOStream();

		std::vector<char> data() const;
};

// Compressed input stream
class GZIStream : public BIStream
{
	public:
		GZIStream(const std::string &path);
};

// Compressed output stream
class GZOStream : public BOStream
{
	public:
		GZOStream(const std::string &path, bool append = false);
};


// Inlined functions
inline BOStream &BOStream::operator<<(char c) {
	ASSERT_WRITE((char *)&c, 1);
	return *this;
}

inline BOStream &BOStream::operator<<(uint32_t i) {
#ifndef WORDS_BIGENDIAN
	i = bswap(i);
#endif
	ASSERT_WRITE((char *)&i, 4);
	return *this;
}

inline BOStream &BOStream::operator<<(uint64_t i) {
#ifndef WORDS_BIGENDIAN
	i = bswap(i);
#endif
	ASSERT_WRITE((char *)&i, 8);
	return *this;
}

inline BOStream &BOStream::operator<<(const std::string &s) {
	ASSERT_WRITE(s.data(), s.length());
	return (*this << '\0');
}

inline BOStream &BOStream::operator<<(const std::vector<char> &v) {
	(*this) << (uint32_t)v.size();
	ASSERT_WRITE(&(v[0]), v.size());
	return *this;
}

inline BIStream &BIStream::operator>>(char &c) {
	ASSERT_READ((char *)&c, 1);
	return *this;
}

inline BIStream &BIStream::operator>>(uint32_t &i) {
	ASSERT_READ((char *)&i, 4);
#ifndef WORDS_BIGENDIAN
	i = bswap(i);
#endif
	return *this;
}

inline BIStream &BIStream::operator>>(uint64_t &i) {
	ASSERT_READ((char *)&i, 8);
#ifndef WORDS_BIGENDIAN
	i = bswap(i);
#endif
	return *this;
}

inline BIStream &BIStream::operator>>(std::string &s) {
	char buffer[120], *bptr = buffer;
	char c;
	s.clear();
	do {
		if (eof()) {
			s.clear();
			return *this;
		}
		(*this) >> c;
		switch (c) {
			case 0: break;
			default: {
				*bptr = c;
				if (bptr-buffer == sizeof(buffer)-1) {
					s.append(buffer, sizeof(buffer));
					bptr = buffer;
				} else {
					++bptr;
				}
				continue;
			}
		}
		break;
	} while (true);

	if (bptr != buffer) {
		s.append(buffer, bptr-buffer);
	}
	return *this;
}

inline BIStream &BIStream::operator>>(std::vector<char> &v) {
	uint32_t size;
	*this >> size;
	v.resize(size);
	ASSERT_READ(&v[0], size);
	return *this;
}


#endif // BSTREAM_H_
