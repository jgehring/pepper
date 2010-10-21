/*
 * pepper - SCM statistics report generator
 * Copyright (C) 2010 Jonas Gehring
 *
 * file: bstream.h
 * Binary input and output streams (interfaces)
 */


#include "bstream.h"


// Constructor
BStream::BStream(FILE *f)
	: m_file(f)
{

}

// Destructor
BStream::~BStream()
{
	if (m_file) {
		fclose(m_file);
	}
}


// Constructor
BIStream::BIStream(const std::string &path)
#if (__GLIBC__ == 2) && (__GLIBC_MINOR__ >= 3)
	: BStream(fopen(path.c_str(), "rbm"))
#else
	: BStream(fopen(path.c_str(), "rb"))
#endif
{

}


// Constructor
BOStream::BOStream(const std::string &path, bool append)
	: BStream(fopen(path.c_str(), (append ? "ab" : "wb")))
{

}
