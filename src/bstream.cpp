/*
 * pepper - SCM statistics report generator
 * Copyright (C) 2010 Jonas Gehring
 *
 * file: bstream.h
 * Binary input and output streams (interfaces)
 */


#include "bstream.h"


// Constructor
BStream::BStream(FilePtr f)
	: m_file(f)
{

}

// Destructor
BStream::~BStream()
{
	if (m_file) {
#ifdef HAVE_LIBZ
		gzclose(m_file);
#else
		fclose(m_file);
#endif
	}
}


// Constructor
BIStream::BIStream(const std::string &path)
#ifdef HAVE_LIBZ
	: BStream(gzopen(path.c_str(), "rb"))
#else
 #if (__GLIBC__ == 2) && (__GLIBC_MINOR__ >= 3)
	: BStream(fopen(path.c_str(), "rbm"))
 #else
	: BStream(fopen(path.c_str(), "rb"))
 #endif
#endif
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
BOStream::BOStream(const std::string &path, bool append)
#ifdef HAVE_LIBZ
	: BStream(append ? NULL : gzopen(path.c_str(), "wb"))
#else
	: BStream(fopen(path.c_str(), (append ? "ab" : "wb")))
#endif
{
#ifdef HAVE_LIBZ
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
		}

		gzclose(in);
		m_file = gzopen(path.c_str(), "wb");
		buf = first;
		while (buf != NULL) {
			gzwrite(m_file, buf->data, buf->size);
			LBuffer *tmp = buf;
			buf = buf->next;
			delete tmp;
		}
	}
#endif
}
