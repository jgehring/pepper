/*
 * pepper - SCM statistics report generator
 * Copyright (C) 2010 Jonas Gehring
 *
 * file: backend.cpp
 * Abstract repository backend
 */


#include "options.h"

#include "backend.h"



// Protected constructor
Backend::Backend(const std::string &url)
{
}

// Allocates a backend for the given options
Backend *Backend::backendFor(const Options &options)
{
	return NULL;
}
