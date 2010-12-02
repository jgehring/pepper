/*
 * pepper - SCM statistics report generator
 * Copyright (C) 2010 Jonas Gehring
 *
 * file: bstream.cpp
 * Binary input and output streams (implementations)
 */


#include <cstdio>

#include "main.h"

#ifdef HAVE_LIBZ
 #include <zlib.h>
#endif

#include "bstream.h"


// RawStream implementation for cstdio
class FileStream : public BStream::RawStream
{
public:
	FileStream(FILE *f) : f(f) { }
	~FileStream() { if (f) fclose(f); }

	bool ok() const {
		return f != NULL;
	}
	bool eof() const {
		return feof(f);
	}
	size_t tell() const {
		return ftell(f);
	}
	bool seek(size_t offset) {
		return (fseek(f, offset, SEEK_SET) >= 0);
	}
	int read(void *ptr, size_t n) {
		return fread(ptr, 1, n, f);
	}
	int write(const void *ptr, size_t n) {
		return fwrite(ptr, 1, n, f);
	}

	FILE *f;
};

// RawStream implementation for memory buffers
class MemoryStream : public BStream::RawStream
{
public:
	MemoryStream() : p(0), size(512) {
		m_buffer = new char[size];
	}
	MemoryStream(const char *data, size_t n) : p(0), size(n) {
		m_buffer = new char[n];
		memcpy(m_buffer, data, n);
	}
	~MemoryStream() { delete[] m_buffer;} 

	bool ok() const {
		return true;
	}
	bool eof() const {
		return !(p < size);
	}
	size_t tell() const {
		return p;
	}
	bool seek(size_t offset) {
		if (offset >= size) {
			return false;
		}
		p = offset;
		return true;
	}
	int read(void *ptr, size_t n) {
		size_t nr = std::min(size - p, n);
		memcpy(ptr, m_buffer + p, nr);
		p += nr;
		return nr;
	}
	int write(const void *ptr, size_t n) {
		if (p + n >= size) {
			size_t oldsize = size;
			do {
				size *= 2;
			} while (p + n >= size);
			char *newbuf = new char[size];
			memcpy(newbuf, m_buffer, oldsize);
			delete[] m_buffer;
			m_buffer = newbuf;
		}
		memcpy(m_buffer + p, ptr, n);
		p += n;
		return n;
	}

	char *m_buffer;
	size_t p, size;
};

#ifdef HAVE_LIBZ

// RawStream implementation for zlib
class GzStream : public BStream::RawStream
{
public:
	GzStream(gzFile f) : f(f) { }
	~GzStream() { if (f) gzclose(f); }

	bool ok() const {
		return f != NULL;
	}
	bool eof() const {
		return gzeof(f);
	}
	size_t tell() const {
		return gztell(f);
	}
	bool seek(size_t offset) {
		return (gzseek(f, offset, SEEK_SET) >= 0);
	}
	int read(void *ptr, size_t n) {
		return gzread(f, ptr, n);
	}
	int write(const void *ptr, size_t n) {
		return gzwrite(f, ptr, n);
	}

	gzFile f;
};

#endif // HAVE_LIBZ


// Constructors
BIStream::BIStream(const std::string &path)
#if (__GLIBC__ == 2) && (__GLIBC_MINOR__ >= 3)
	: BStream(new FileStream(fopen(path.c_str(), "rbm")))
#else
	: BStream(new FileStream(fopen(path.c_str(), "rb")))
#endif
{
}

BIStream::BIStream(FILE *f)
	: BStream(new FileStream(f))
{
}

BIStream::BIStream(RawStream *stream)
	: BStream(stream)
{
}

BOStream::BOStream(const std::string &path, bool append)
	: BStream(new FileStream(fopen(path.c_str(), (append ? "ab" : "wb"))))
{
}

BOStream::BOStream(RawStream *stream)
	: BStream(stream)
{
}

MIStream::MIStream(const char *data, size_t n)
	: BIStream(new MemoryStream(data, n))
{
}

MIStream::MIStream(const std::vector<char> &data)
	: BIStream(new MemoryStream(&data[0], data.size()))
{
}

MOStream::MOStream()
	: BOStream(new MemoryStream())
{
}

// Returns the stream's internal buffer
std::vector<char> MOStream::data() const
{
	MemoryStream *ms = (MemoryStream *)m_stream;
	return std::vector<char>(ms->m_buffer, ms->m_buffer + ms->p);
}

#ifdef HAVE_LIBZ

// Constructors
GZIStream::GZIStream(const std::string &path)
	: BIStream(new GzStream(gzopen(path.c_str(), "rb")))
{
}

// Single-linked buffer
struct LBuffer {
	char *data;
	int size;
	LBuffer *next;
	LBuffer() : data(NULL), size(0), next(NULL) { }
	~LBuffer() { delete[] data; }
};

// Constructor
GZOStream::GZOStream(const std::string &path, bool append)
	: BOStream(append ? NULL : new GzStream(gzopen(path.c_str(), "wb")))
{
	// Appending to gzipped files does not work. Instead, read the whole
	// file and re-write it.
	if (append) {
		LBuffer *buf = NULL, *first = NULL;
		gzFile in = gzopen(path.c_str(), "rb");
		if (in) {
			while (!gzeof(in)) {
				if (buf) {
					buf->next = new LBuffer();
					buf = buf->next;
				} else {
					buf = new LBuffer();
					first = buf;
				}
				buf->data = new char[4096];
				buf->size = gzread(in, buf->data, 4096);
			}
			gzclose(in);
		}

		gzFile gz = gzopen(path.c_str(), "wb");
		buf = first;
		while (buf != NULL) {
			gzwrite(gz, buf->data, buf->size);
			LBuffer *tmp = buf;
			buf = buf->next;
			delete tmp;
		}

		m_stream = new GzStream(gz);
	}
}

#endif // HAVE_LIBZ
