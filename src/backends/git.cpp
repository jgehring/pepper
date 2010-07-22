/*
 * pepper - SCM statistics report generator
 * Copyright (C) 2010 Jonas Gehring
 *
 * file: git.cpp
 * Git repository backend
 */


#include "backends/git.h"


// Constructor
GitBackend::GitBackend(const Options &options)
	: Backend(options)
{

}

// Destructor
GitBackend::~GitBackend()
{

}

// Returns the revision data for the given ID
Revision *GitBackend::revision(const std::string &id)
{
	return NULL;
}
