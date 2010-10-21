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


// Constructor
BOStream::BOStream(const std::string &path, bool append)
#ifdef HAVE_LIBZ
	: BStream(gzopen(path.c_str(), (append ? "ab" : "wb")))
#else
	: BStream(fopen(path.c_str(), (append ? "ab" : "wb")))
#endif
{

}
