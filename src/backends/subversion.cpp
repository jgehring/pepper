/*
 * pepper - SCM statistics report generator
 * Copyright (C) 2010 Jonas Gehring
 *
 * file: subversion.cpp
 * Subversion repository backend
 */


#include "backends/subversion.h"


// Constructor
SubversionBackend::SubversionBackend(const Options &options)
	: Backend(options)
{

}

// Destructor
SubversionBackend::~SubversionBackend()
{

}

// Returns the revision data for the given ID
Revision *SubversionBackend::revision(const std::string &id)
{
	// TODO
	return NULL;
}
